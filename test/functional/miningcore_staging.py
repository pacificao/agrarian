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

        node, peer = Node(work / "node", network="regtest"), Node(work / "peer", network="regtest")
        resources.callback(node.stop)
        resources.callback(peer.stop)
        node.start(staking=False)
        peer.start(staking=False)
        peer.connect(node)
        node.rpc("generate", 25)
        wait_for(lambda: peer.rpc("getblockcount") == 25)
        pool_address = node.rpc("getnewaddress", raw=True)
        miner_address = peer.rpc("getnewaddress", raw=True)
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
                "ports": {str(stratum_port): {"listenAddress": "127.0.0.1", "difficulty": 0.001}},
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
        block = mine_one(stratum_port, miner_address, helper, node, 26,
                         args.miningcore.with_name("libmultihash.so"))
        wait_for(lambda: peer.rpc("getbestblockhash", raw=True) == block)
        wait_for(lambda: sql("SELECT count(*) FROM shares WHERE blockheight = 26") == "1")
        wait_for(lambda: sql("SELECT hash FROM blocks WHERE blockheight = 26") == block)
        print(f"Miningcore accepted and persisted a Stratum share, and both nodes accepted block 26: {block}", flush=True)
        node.rpc("generate", 25)
        wait_for(lambda: peer.rpc("getblockcount") == 51)
        wait_for(lambda: sql("SELECT status FROM blocks WHERE blockheight = 26") == "confirmed", timeout=100)
        txid = wait_for(lambda: sql("SELECT transactionconfirmationdata FROM payments LIMIT 1"), timeout=90)
        node.rpc("generate", 1)
        wait_for(lambda: peer.rpc("getblockcount") == 52)
        assert peer.rpc("gettransaction", txid)["confirmations"] >= 1
        amount = peer.rpc("getreceivedbyaddress", miner_address, 1)
        assert amount > 0
        print(f"Miningcore payout reached the separate miner wallet: {amount} test AGR, tx {txid}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("dotnet", "miningcore", "pg-bin", "schema"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    if {name for _, name in socket.if_nameindex()} != {"lo"}:
        parser.error("run this test in a loopback-only network namespace")
    for key in vars(args):
        setattr(args, key, getattr(args, key).resolve(strict=True))
    digest = hashlib.sha256(args.miningcore.read_bytes()).hexdigest()
    print(f"Staging Miningcore binary SHA256: {digest}", flush=True)
    with tempfile.TemporaryDirectory(prefix="agrarian-miningcore-") as directory:
        work = Path(directory)
        try:
            run(args, work)
        except Exception:
            for name in ("postgres.log", "miningcore.log", "node/regtest/debug.log", "peer/regtest/debug.log"):
                log = work / name
                if log.exists():
                    print(f"--- {name} ---")
                    lines = log.read_text(errors="replace").splitlines()
                    print("\n".join(lines[:45] + lines[-20:] if name == "miningcore.log" else lines[-15:]))
            raise


if __name__ == "__main__":
    main()
