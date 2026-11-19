#!/usr/bin/env python3
"""Offline smoke test for an installed Agrarian desktop package on Linux or Windows."""

import argparse
import base64
import json
import secrets
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def wait_for(check, proc, timeout=90):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"Agrarian exited unexpectedly: {proc.returncode}")
        try:
            return check()
        except (URLError, ConnectionError, TimeoutError, RuntimeError):
            time.sleep(0.5)
    raise RuntimeError("Timed out waiting for wallet RPC")


def test(qt, daemon, root):
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    password = secrets.token_hex(32)
    passphrase = secrets.token_hex(32)
    credential = base64.b64encode(f"package-test:{password}".encode()).decode()

    def rpc(method, *params):
        request = Request(f"http://127.0.0.1:{port}/", headers={
            "Authorization": f"Basic {credential}", "Content-Type": "application/json",
        }, data=json.dumps({"id": "package-test", "method": method, "params": params}).encode())
        try:
            response = urlopen(request, timeout=5)
        except HTTPError as exc:
            response = exc
        with response:
            message = json.load(response)
        if message["error"]:
            raise RuntimeError(f"{method}: {message['error']['message']}")
        return message["result"]

    def start(binary, path, *extra):
        return subprocess.Popen([str(binary), "-regtest", f"-datadir={path}",
            "-server=1", "-listen=0", "-connect=0", "-dnsseed=0", "-discover=0",
            "-upnp=0", "-listenonion=0", "-staking=0", "-keypool=5",
            "-rpcbind=127.0.0.1", f"-rpcport={port}", *extra])

    def stop(proc):
        if proc.poll() is None:
            try:
                rpc("stop")
                proc.wait(timeout=25)
            except (URLError, ConnectionError, TimeoutError, RuntimeError, subprocess.TimeoutExpired):
                proc.kill()
                proc.wait(timeout=15)

    def datadir(name):
        path = root / name
        path.mkdir()
        (path / "agrarian.conf").write_text(f"rpcuser=package-test\nrpcpassword={password}\n")
        return path

    gui_directory = datadir("gui")
    gui = start(qt, gui_directory, "-choosedatadir=0")
    try:
        wait_for(lambda: rpc("getwalletinfo"), gui)
        assert rpc("getblockchaininfo")["chain"] == "regtest"
        assert rpc("validateaddress", rpc("getnewaddress"))["ismine"]
    finally:
        stop(gui)

    path = datadir("wallet")
    backup = root / "encrypted-wallet.dat"
    proc = start(daemon, path)
    try:
        wait_for(lambda: rpc("getwalletinfo"), proc)
        assert rpc("getblockchaininfo")["chain"] == "regtest"
        address = rpc("getnewaddress")
        assert len(rpc("generate", 101)) == 101
        txid = rpc("sendtoaddress", address, 1)
        rpc("generate", 1)
        assert rpc("gettransaction", txid)["confirmations"] == 1
        rpc("encryptwallet", passphrase)
        assert proc.wait(timeout=45) == 0
        proc = start(daemon, path)
        wait_for(lambda: rpc("getwalletinfo"), proc)
        assert rpc("getwalletinfo")["unlocked_until"] == 0
        rpc("walletpassphrase", passphrase, 60)
        signature = rpc("signmessage", address, "Agrarian release test")
        assert rpc("verifymessage", address, signature, "Agrarian release test")
        rpc("backupwallet", str(backup))
        assert backup.stat().st_size > 0
    finally:
        stop(proc)

    shutil.copyfile(backup, path / "regtest" / "wallet.dat")
    proc = start(daemon, path)
    try:
        wait_for(lambda: rpc("getwalletinfo"), proc)
        assert rpc("getwalletinfo")["unlocked_until"] == 0
        assert rpc("gettransaction", txid)["confirmations"] == 1
        rpc("walletpassphrase", passphrase, 60)
        assert rpc("signmessage", address, "Agrarian release test") == signature
    finally:
        stop(proc)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qt", required=True, type=Path)
    parser.add_argument("--daemon", required=True, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="agrarian-package-test-") as directory:
        test(args.qt.resolve(strict=True), args.daemon.resolve(strict=True), Path(directory))
    print("Packaged GUI launch, transaction, encryption, restart and backup restore passed")
