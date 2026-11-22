"""Test the installed Qt process and miner against a loopback-only Stratum fixture."""

import base64
import json
import re
import secrets
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from urllib.error import URLError
from urllib.request import ProxyHandler, Request, build_opener


def wait_for(predicate, timeout=30):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        result = predicate()
        if result:
            return result
        time.sleep(0.25)
    raise AssertionError(f"Timed out: {predicate.__name__}")


def exercise(qt, data, pool):
    with socket.socket() as reserved_rpc:
        reserved_rpc.bind(("127.0.0.1", 0))
        rpc_port = reserved_rpc.getsockname()[1]
    stratum_port = pool.getsockname()[1]
    password = secrets.token_hex(32)
    (data / "agrarian.conf").write_text(f"rpcuser=wallet-qa\nrpcpassword={password}\n", encoding="ascii")
    auth = base64.b64encode(f"wallet-qa:{password}".encode()).decode()
    http = build_opener(ProxyHandler({}))

    def rpc(method, *params):
        request = Request(f"http://127.0.0.1:{rpc_port}/", headers={
            "Authorization": f"Basic {auth}", "Content-Type": "application/json",
        }, data=json.dumps({"id": "qa", "method": method, "params": params}).encode())
        with http.open(request, timeout=3) as response:
            return json.load(response)["result"]

    wallet = subprocess.Popen([
        str(qt), "-regtest", "-lang=en", f"-datadir={data}", "-choosedatadir=0",
        "-windowtitle=Windows mining QA", "-server=1", "-listen=0", "-connect=0",
        "-dnsseed=0", "-discover=0", "-upnp=0", "-listenonion=0", "-staking=0",
        "-keypool=5", "-par=1", "-rpcbind=127.0.0.1", f"-rpcport={rpc_port}",
        f"-regtestpoolminingport={stratum_port}",
    ])

    def control(action):
        script = Path(__file__).with_suffix(".ps1")
        run = subprocess.run(
            ["powershell.exe", "-NoProfile", "-NonInteractive", "-File", str(script),
             "-WalletProcess", str(wallet.pid), "-Action", action],
            timeout=25, text=True, capture_output=True,
        )
        if run.returncode:
            raise RuntimeError(f"{action}: {run.stderr.strip()}")
        return run.stdout.strip()

    def ready():
        if wallet.poll() is not None:
            raise AssertionError(f"Installed Qt wallet exited: {wallet.returncode}")
        try:
            return rpc("getblockchaininfo").get("chain") == "regtest" and rpc("getwalletinfo")
        except (URLError, TimeoutError):
            return False

    def no_worker():
        return not control("child")

    def start_worker(action, expected_address=None):
        control("start")
        pool.settimeout(25)
        conn, peer = pool.accept()
        with conn, conn.makefile("rwb", buffering=0) as wire:
            assert peer[0] == "127.0.0.1"
            conn.settimeout(10)

            def exchange(method, result):
                line = wire.readline(4096)
                message = json.loads(line)
                assert message["method"] == method
                wire.write(json.dumps({"id": message["id"], "result": result, "error": None}).encode() + b"\n")
                return message["params"]

            exchange("mining.subscribe", [[["mining.notify", "local-qa"]], "ab12cd34", 4])
            worker, marker = exchange("mining.authorize", True)
            address, suffix = worker.split(".")
            assert suffix == "agrarian-qt" and marker == "x"
            assert rpc("validateaddress", address)["ismine"]
            assert expected_address is None or address == expected_address
            child = wait_for(lambda: control("child"))
            _, actual, cmd = child.split("|", 2)
            assert Path(actual).resolve() == qt.parent / "miners" / "cpuminer.exe", actual
            assert f"stratum+tcp://127.0.0.1:{stratum_port}" in cmd
            assert re.search(r" -a quark ", cmd) and re.search(r" -t 1 -b 0$", cmd)
            # No mining.notify messages are sent: this service never assigns work.
            if action == "exit":
                rpc("stop")
                wallet.wait(timeout=45)
            else:
                control(action)
            # Windows may reset the TCP socket when Qt terminates the
            # console helper. Require an exited process independently.
            try:
                while extra := wire.readline(4096):
                    assert json.loads(extra)["method"] == "mining.extranonce.subscribe"
            except ConnectionResetError:
                pass
            wait_for(no_worker, timeout=15)
            print(f"Windows Qt {action}: exact miner, owned payout address, and clean disconnect verified", flush=True)
            return address

    try:
        wait_for(ready, timeout=120)
        assert no_worker(), "Mining auto-started on wallet launch"
        control("open")
        assert no_worker(), "Mining auto-started upon opening the pool dialog"
        address = start_worker("stop")
        start_worker("close", address)
        control("open")
        start_worker("exit", address)
        assert wallet.returncode == 0
    finally:
        if wallet.poll() is None:
            try:
                rpc("stop")
                wallet.wait(timeout=20)
            except (URLError, TimeoutError, subprocess.TimeoutExpired):
                wallet.kill()
                wallet.wait(timeout=10)


if __name__ == "__main__":
    wallet_binary = Path(sys.argv[1]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="agrarian-windows-mining-") as temp, socket.socket() as fixture:
        fixture.bind(("127.0.0.1", 0))
        fixture.listen()
        exercise(wallet_binary, Path(temp), fixture)
