#!/usr/bin/env python3
"""Drive an isolated Miningcore instance through Stratum and a regtest payout.

Run in a loopback-only network namespace. Uses disposable PostgreSQL and
wallet data; never pass a production config or connection string.
"""

import argparse
import ctypes
import hashlib
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import tempfile
import time
from contextlib import ExitStack
from pathlib import Path

from testnet_hybrid import Node, build_solver, sha256d, wait_for


def port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def stop(process):
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=20)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)


class DesktopMiner:
    """Operate the installed Qt wallet on the private X11 display."""

    def __init__(self, wallet, binary):
        self.wallet = wallet
        self.binary = binary
        self.pid = wallet.proc.pid
        self.main = wait_for(lambda: self.window("Agrarian Core - Wallet"))
        self.dialog = None

    def xdo(self, *args, check=True):
        return subprocess.run(["xdotool", *map(str, args)], check=check,
                              capture_output=True, text=True, timeout=5)

    def window(self, title):
        result = self.xdo("search", "--all", "--onlyvisible", "--pid", self.pid,
                          "--name", title, check=False)
        windows = result.stdout.splitlines()
        return windows[0] if windows else None

    def helpers(self):
        children = Path(f"/proc/{self.pid}/task/{self.pid}/children").read_text().split()
        result = []
        for child in children:
            try:
                if Path(f"/proc/{child}/exe").resolve(strict=True) == self.binary:
                    result.append(int(child))
            except FileNotFoundError:
                continue
        return result

    def keys(self, window, *keys):
        self.xdo("windowfocus", "--sync", window, "key", "--clearmodifiers", *keys)

    def button(self, x):
        # The test uses the stock English dialog at 1x scale. Click instead
        # of sending Alt+S, which the main wallet's Settings menu also uses.
        geometry = subprocess.check_output(["xwininfo", "-id", self.dialog], text=True, timeout=5)
        left = int(re.search(r"Absolute upper-left X:\s+(-?\d+)", geometry)[1])
        top = int(re.search(r"Absolute upper-left Y:\s+(-?\d+)", geometry)[1])
        self.xdo("windowfocus", "--sync", self.dialog, "mousemove", left + x, top + 162)
        self.xdo("click", 1)

    def start(self):
        if not self.dialog:
            assert not self.helpers(), "Miner started before opening the menu"
            self.keys(self.main, "alt+m", "a")
            self.dialog = wait_for(lambda: self.window("All The Acronyms"), timeout=8)
            assert not self.helpers(), "Opening mining settings auto-started the worker"
        self.button(100)
        return wait_for(lambda: next(iter(self.helpers()), None), timeout=15)

    def stop(self):
        self.button(390)
        wait_for(lambda: not self.helpers(), timeout=8)

    def close(self):
        self.keys(self.dialog, "Escape")
        wait_for(lambda: not self.helpers() and not self.window("All The Acronyms"), timeout=8)


def miningcore_quark(multihash, header):
    # Miningcore resolves native algorithms lazily; unrelated hash symbols in
    # the shared object are not needed by this test.
    ctypes.CDLL("libcrypto.so.3", mode=ctypes.RTLD_GLOBAL)
    loader = ctypes.CDLL(None)
    loader.dlopen.argtypes = (ctypes.c_char_p, ctypes.c_int)
    loader.dlopen.restype = ctypes.c_void_p
    loader.dlsym.argtypes = (ctypes.c_void_p, ctypes.c_char_p)
    loader.dlsym.restype = ctypes.c_void_p
    loader.dlclose.argtypes = (ctypes.c_void_p,)
    handle = loader.dlopen(bytes(multihash), os.RTLD_LAZY)
    assert handle, "Could not load Miningcore's native library"
    try:
        symbol = loader.dlsym(handle, b"quark_export")
        assert symbol, "Miningcore's Quark symbol was not exported"
        quark = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_uint)(symbol)
        result = ctypes.create_string_buffer(32)
        quark(header, result, len(header))
        return result.raw
    finally:
        loader.dlclose(handle)


def mine_one(stratum_port, address, helper, node, height, multihash):
    with socket.create_connection(("127.0.0.1", stratum_port), timeout=15) as sock, sock.makefile("rwb") as stream:
        jobs = []

        def call(number, method, params):
            stream.write(json.dumps({"id": number, "method": method, "params": params}).encode() + b"\n")
            stream.flush()
            while True:
                message = read()
                if message.get("id") == number:
                    assert not message.get("error"), message
                    return message.get("result")

        def read():
            line = stream.readline()
            assert line, "Stratum disconnected"
            message = json.loads(line)
            if message.get("method") == "mining.notify":
                jobs.append(message["params"])
            return message

        _, extra1, extra2_size = call(1, "mining.subscribe", ["Agrarian staging miner/1.0"])
        worker = address + ".staging"
        assert call(2, "mining.authorize", [worker, "x"]) is True
        while not jobs:
            read()
        job, prev_stratum, coinbase1, coinbase2, branches, version, bits, ntime, _ = jobs[-1]
        extra2 = "00" * extra2_size
        coinbase = bytes.fromhex(coinbase1 + extra1 + extra2 + coinbase2)
        root = sha256d(coinbase)
        for branch in branches:
            root = sha256d(root + bytes.fromhex(branch))
        tip = node.rpc("getbestblockhash", raw=True)
        prev = bytes.fromhex(tip)[::-1]
        assert b"".join(prev[i:i+4][::-1] for i in range(0, 32, 4)).hex() == prev_stratum
        header = struct.pack("<I", int(version, 16)) + prev + root + struct.pack(
            "<III", int(ntime, 16), int(bits, 16), 0,
        )
        compact = int(bits, 16)
        target = (compact & 0x7fffff) << (8 * ((compact >> 24) - 3))
        nonce = int(subprocess.run([str(helper)], input=header + target.to_bytes(32, "big"),
                                   check=True, capture_output=True, timeout=120).stdout)
        solved_header = header[:76] + struct.pack("<I", nonce)
        pool_hash = miningcore_quark(multihash, solved_header)
        assert int.from_bytes(pool_hash, "little") <= target, (
            f"Miningcore Quark disagreed with Core for the same header: {pool_hash[::-1].hex()}"
        )
        assert call(3, "mining.submit", [worker, job, extra2, ntime, f"{nonce:08x}"]) is True
        try:
            wait_for(lambda: node.rpc("getblockcount") == height, timeout=20)
        except AssertionError:
            # Independent diagnostic only: never count direct submission as
            # a passing Stratum block or proceed to a pool payout.
            template = node.rpc("getblocktemplate")
            assert template["height"] == height and template["transactions"] == []
            checkpoint = bytes.fromhex(template["acc_checkpoint"])[::-1]
            payload = solved_header + checkpoint + b"\x01" + coinbase
            reply = node.rpc("submitblock", payload.hex(), raw=True)
            raise AssertionError(f"Pool accepted only a share; direct Core submission: {reply or 'accepted'}")
        return node.rpc("getbestblockhash", raw=True)


def run(args, work):
    with ExitStack() as resources:
        def launch(command, log):
            stdout = resources.enter_context((work / log).open("w"))
            child = subprocess.Popen(command, stdout=stdout, stderr=subprocess.STDOUT)
            resources.callback(stop, child)
            return child

        pg_port, stratum_port = port(), port()
        pg_data = work / "postgres"
        subprocess.run([str(args.pg_bin / "initdb"), "-D", str(pg_data), "--locale=C",
                        "--encoding=UTF8", "--username=miningcore", "--auth=trust"],
                       check=True, capture_output=True)
        pg = launch([str(args.pg_bin / "postgres"), "-D", str(pg_data), "-h", "127.0.0.1",
                     "-p", str(pg_port), "-k", str(work), "-F"], "postgres.log")
        args_psql = [str(args.pg_bin / "psql"), "-X", "-h", "127.0.0.1", "-p", str(pg_port),
                     "-U", "miningcore", "-d", "postgres", "-At", "-v", "ON_ERROR_STOP=1"]

        def sql(statement):
            return subprocess.run([*args_psql, "-c", statement], check=True, capture_output=True,
                                  text=True, timeout=8).stdout.strip()

        def ready():
            assert pg.poll() is None
            try:
                return sql("SELECT 1") == "1"
            except subprocess.CalledProcessError:
                return False

        wait_for(ready, timeout=30)
        subprocess.run([*args_psql, "-f", str(args.schema)], check=True, capture_output=True)

        binaries = {}
        if args.core_bin:
            binaries = {"daemon": [str(args.core_bin / "agrariand")], "cli": args.core_bin / "agrarian-cli"}
        node = Node(work / "node", network="regtest", **binaries)
        peer_binaries = binaries
        if args.qt_wallet:
            peer_binaries = {**binaries, "daemon": [str(args.qt_wallet), "-choosedatadir=0", "-lang=en"]}
        peer = Node(work / "peer", network="regtest", **peer_binaries)
        resources.callback(node.stop)
        resources.callback(peer.stop)
        flags = ("-regtestsplitdifficultyheight=26",) if args.split_difficulty else ()
        node.start(staking=False, extra=flags)
        qt_flags = (f"-regtestpoolminingport={stratum_port}",) if args.qt_wallet else ()
        peer.start(staking=False, extra=flags + qt_flags)
        peer.connect(node)
        node.rpc("generate", 25)
        wait_for(lambda: peer.rpc("getblockcount") == 25)
        template_bits = None
        if args.split_difficulty:
            scheduled = node.rpc("getblockchaininfo")["hybrid"]
            assert scheduled["activation_height"] == 26 and scheduled["split_targets_for_next_block"]
            assert not scheduled["normalized_fork_choice_at_tip"]
            template_bits = node.rpc("getblocktemplate")["bits"]
            assert template_bits == scheduled["next_pow_bits"]
        pool_address = node.rpc("getnewaddress", raw=True)
        miner_address = None if args.qt_wallet else peer.rpc("getnewaddress", raw=True)
        if not (args.cpuminer or args.first_party_worker):
            helper = work / "pow"
            build_solver(helper)
        config = {
            "clusterName": "agrarian-isolated-staging",
            "logging": {"level": "info", "enableConsoleLog": True,
                        "logBaseDirectory": str(work), "perPoolLogFile": False},
            "api": {"enabled": False},
            "banning": {"manager": "Integrated", "banOnInvalidShares": False},
            "notifications": {"enabled": False, "email": {}, "admin": {"enabled": False}},
            "persistence": {"postgres": {"host": "127.0.0.1", "port": pg_port, "user": "miningcore",
                                         "password": node.rpc_password, "database": "postgres"}},
            "paymentProcessing": {"enabled": True, "interval": 2,
                                  "shareRecoveryFile": str(work / "recovered-shares.txt")},
            "pools": [{
                "id": "agrarian", "coin": "agrarian", "enabled": True, "address": pool_address,
                "rewardRecipients": [],
                "blockRefreshInterval": 500, "jobRebroadcastTimeout": 180, "clientConnectionTimeout": 240,
                "banning": {"enabled": False, "checkThreshold": 50, "invalidPercent": 50, "time": 600},
                "ports": {str(stratum_port): {"listenAddress": "127.0.0.1",
                                            "difficulty": 0.0001 if (args.cpuminer or args.first_party_worker) else 0.001}},
                "daemons": [{"host": "127.0.0.1", "port": node.rpc_port, "user": node.rpc_user,
                             "password": node.rpc_password, "minimumConfirmations": 22}],
                "paymentProcessing": {"enabled": True, "minimumPayment": 0.1, "payoutScheme": "PPLNS",
                                      "payoutSchemeConfig": {"factor": 2}, "minersPayTxFees": False},
            }],
        }
        config_file = work / "miningcore.json"
        config_file.write_text(json.dumps(config))
        pool = launch([str(args.dotnet), str(args.miningcore), "-c", str(config_file)], "miningcore.log")

        def stratum_ready():
            assert pool.poll() is None, "Miningcore stopped"
            try:
                with socket.create_connection(("127.0.0.1", stratum_port), timeout=0.5):
                    return True
            except OSError:
                return False

        wait_for(stratum_ready, timeout=60)
        if args.cpuminer or args.first_party_worker:
            # The real desktop worker is deliberately scoped to a single
            # local Stratum endpoint in a network namespace with no egress.
            second_worker = None
            if args.qt_wallet:
                desktop = DesktopMiner(peer, args.cpuminer)
                try:
                    worker = desktop.start()
                except AssertionError:
                    if args.qt_failure_image and shutil.which("import"):
                        subprocess.run(["import", "-window", "root", str(args.qt_failure_image)], timeout=8)
                    raise
                assert desktop.helpers() == [worker]
                cmdline = Path(f"/proc/{worker}/cmdline").read_bytes().decode().split("\0")[:-1]
                assert cmdline[1:6] == ["-a", "quark", "-o", f"stratum+tcp://127.0.0.1:{stratum_port}", "-u"]
                assert cmdline[7:] == ["-p", "x", "-t", "1", "-b", "0"]
                miner_address, suffix = cmdline[6].split(".")
                assert suffix == "agrarian-qt"
                assert peer.rpc("validateaddress", miner_address)["ismine"]
            elif args.first_party_worker:
                mode = "--regtest-opencl-software" if args.software_opencl_kernel else "--regtest-loopback"
                command = [str(args.first_party_worker), mode, str(stratum_port), miner_address]
                if args.software_opencl_kernel:
                    command.append(str(args.software_opencl_kernel))
                worker = launch(command, "cpuminer.log")
                if args.simultaneous:
                    second_worker = launch([str(args.first_party_worker), "--regtest-loopback",
                                            str(stratum_port), miner_address], "cpu-parallel.log")
            else:
                worker = launch([str(args.cpuminer), "-a", "quark", "-o",
                                 f"stratum+tcp://127.0.0.1:{stratum_port}",
                                 "-u", miner_address + ".agrarian-qt", "-p", "x",
                                 "-t", "1", "-b", "0"], "cpuminer.log")
            try:
                def found_block():
                    running = worker in desktop.helpers() if args.qt_wallet else worker.poll() is None
                    assert running, "Desktop CPU miner stopped before finding a block"
                    if second_worker:
                        assert second_worker.poll() is None, "Parallel CPU worker stopped unexpectedly"
                    log_tail = (work / "miningcore.log").read_text(errors="replace").splitlines()[-40:]
                    failures = [line for line in log_tail if "submission failed with:" in line]
                    assert not failures, failures[-1] if failures else ""
                    if node.rpc("getblockcount") < 26:
                        return False
                    if not second_worker:
                        return True
                    return sql(
                        "SELECT count(DISTINCT worker) FROM shares "
                        "WHERE worker IN ('agrarian-qt-cpu', 'agrarian-qt-gpu')"
                    ) == "2"

                wait_for(found_block, timeout=600)
            finally:
                if args.qt_wallet:
                    desktop.stop()
                else:
                    stop(worker)
                    if second_worker:
                        stop(second_worker)
            if args.qt_wallet:
                desktop.start()
                desktop.close()
                print("Qt start, stop, close, default threads and wallet-owned payout address verified", flush=True)
            else:
                assert worker.poll() is not None, "Desktop CPU miner did not stop"
            block = node.rpc("getblockhash", 26, raw=True)
            worker_name = "agrarian-qt"
            if args.first_party_worker:
                worker_name += "-gpu" if args.software_opencl_kernel else "-cpu"
            if args.simultaneous:
                # Both workers are started with the same disposable payout
                # address; verify the stored shares reflect that.
                assert sql(
                    "SELECT count(DISTINCT miner) FROM shares "
                    "WHERE worker IN ('agrarian-qt-cpu', 'agrarian-qt-gpu')"
                ) == "1"
                print("Simultaneous CPU and software OpenCL sessions both submitted accepted shares "
                      "to the same disposable payout address.", flush=True)
            wait_for(lambda: int(sql(
                "SELECT count(*) FROM shares WHERE blockheight = 26 AND " +
                ("worker IN ('agrarian-qt-cpu', 'agrarian-qt-gpu')" if args.simultaneous else
                 f"worker = '{worker_name}'")
            )) > 0)
        else:
            block = mine_one(stratum_port, miner_address, helper, node, 26,
                             args.miningcore.with_name("libmultihash.so"))
            wait_for(lambda: sql("SELECT count(*) FROM shares WHERE blockheight = 26") == "1")
        wait_for(lambda: peer.rpc("getblockcount") >= 26)
        assert peer.rpc("getblockhash", 26, raw=True) == block
        if args.split_difficulty:
            assert node.rpc("getblock", block, "true")["bits"] == template_bits
            assert peer.rpc("getblockchaininfo")["hybrid"]["normalized_fork_choice_at_tip"]
        wait_for(lambda: sql("SELECT hash FROM blocks WHERE blockheight = 26") == block)
        print(f"Miningcore accepted and persisted a Stratum share, and both nodes accepted block 26: {block}", flush=True)
        tip = node.rpc("getblockcount")
        node.rpc("generate", 25)
        wait_for(lambda: peer.rpc("getblockcount") == tip + 25)
        wait_for(lambda: sql("SELECT status FROM blocks WHERE blockheight = 26") == "confirmed", timeout=100)
        txid = wait_for(lambda: sql("SELECT transactionconfirmationdata FROM payments LIMIT 1"), timeout=90)
        node.rpc("generate", 1)
        wait_for(lambda: peer.rpc("getblockcount") == tip + 26)
        assert peer.rpc("gettransaction", txid)["confirmations"] >= 1
        amount = peer.rpc("getreceivedbyaddress", miner_address, 1)
        assert amount > 0
        print(f"Miningcore payout reached the separate miner wallet: {amount} test AGR, tx {txid}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dotnet", "miningcore", "pg-bin", "schema"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--split-difficulty", action="store_true", help="mine and pay out the staged regtest activation block")
    workers = parser.add_mutually_exclusive_group()
    workers.add_argument("--cpuminer", type=Path, help="use the bundled desktop CPU miner instead of the test solver")
    workers.add_argument("--first-party-worker", type=Path, help="use the Agrarian loopback-only worker")
    parser.add_argument("--software-opencl-kernel", type=Path,
                        help="exercise the first-party worker using a CPU OpenCL driver (QA only)")
    parser.add_argument("--simultaneous", action="store_true",
                        help="run CPU and software OpenCL workers against the same regtest pool and wallet")
    parser.add_argument("--core-bin", type=Path, help="directory containing the daemon and CLI to test")
    parser.add_argument("--qt-wallet", type=Path, help="start and stop the installed miner through Qt on a private X11 display")
    parser.add_argument("--qt-failure-image", type=Path, help="write an optional screenshot of the disposable wallet if the test fails")
    args = parser.parse_args()
    if {name for _, name in socket.if_nameindex()} != {"lo"}:
        parser.error("run this test in a loopback-only network namespace")
    for key in ("dotnet", "miningcore", "pg_bin", "schema"):
        setattr(args, key, getattr(args, key).resolve(strict=True))
    for key in ("cpuminer", "first_party_worker", "software_opencl_kernel", "core_bin", "qt_wallet"):
        if getattr(args, key):
            setattr(args, key, getattr(args, key).resolve(strict=True))
    if args.qt_wallet and (not args.core_bin or not args.cpuminer or not os.environ.get("DISPLAY")):
        parser.error("--qt-wallet requires --core-bin, --cpuminer, and a private X11 display")
    if args.qt_wallet and args.cpuminer != args.qt_wallet.parent / "miners/cpuminer":
        parser.error("--cpuminer must be the Qt wallet's bundled helper")
    if args.software_opencl_kernel and not args.first_party_worker:
        parser.error("--software-opencl-kernel requires --first-party-worker")
    if args.simultaneous and (not args.software_opencl_kernel or args.qt_wallet):
        parser.error("--simultaneous requires the first-party software OpenCL fixture without Qt")
    digest = hashlib.sha256(args.miningcore.read_bytes()).hexdigest()
    print(f"Staging Miningcore binary SHA256: {digest}", flush=True)
    with tempfile.TemporaryDirectory(prefix="agrarian-miningcore-") as directory:
        work = Path(directory)
        try:
            run(args, work)
        except Exception:
            for name in ("postgres.log", "miningcore.log", "cpuminer.log", "cpu-parallel.log",
                         "node/regtest/debug.log", "peer/regtest/debug.log"):
                log = work / name
                if log.exists():
                    print(f"--- {name} ---")
                    lines = log.read_text(errors="replace").splitlines()
                    print("\n".join(lines[:45] + lines[-20:] if name == "miningcore.log" else lines[-15:]))
            raise


if __name__ == "__main__":
    main()
