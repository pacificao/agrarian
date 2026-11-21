#!/usr/bin/env python3
"""Offline smoke test for an installed Agrarian desktop package on Linux or Windows."""

import argparse
import base64
import hashlib
import json
import os
import secrets
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import ProxyHandler, Request, build_opener


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


def test(qt, daemon, root, installer=None):
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    password = secrets.token_hex(32)
    passphrase = secrets.token_hex(32)
    credential = base64.b64encode(f"package-test:{password}".encode()).decode()
    opener = build_opener(ProxyHandler({}))

    def rpc(method, *params, timeout=5):
        request = Request(f"http://127.0.0.1:{port}/", headers={
            "Authorization": f"Basic {credential}", "Content-Type": "application/json",
        }, data=json.dumps({"id": "package-test", "method": method, "params": params}).encode())
        try:
            response = opener.open(request, timeout=timeout)
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
            "-upnp=0", "-listenonion=0", "-staking=0", "-keypool=5", "-par=1",
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

    def install_again(expected):
        # Without /D, the installer must reuse the path from the first install.
        result = subprocess.run([str(installer), "/S"], timeout=90)
        assert result.returncode == expected, f"Installer returned {result.returncode}, expected {expected}"

    def digest(path):
        return hashlib.sha256(path.read_bytes()).digest()

    gui_directory = datadir("gui")
    gui = start(qt, gui_directory, "-choosedatadir=0")
    try:
        wait_for(lambda: rpc("getwalletinfo"), gui)
        assert rpc("getblockchaininfo")["chain"] == "regtest"
        gui_address = rpc("getnewaddress")
        assert rpc("validateaddress", gui_address)["ismine"]
        if installer:
            binary_hashes = (digest(qt), digest(daemon))
            install_again(2)
            assert gui.poll() is None
            assert (digest(qt), digest(daemon)) == binary_hashes
    finally:
        stop(gui)

    if installer:
        import winreg

        def wallet_setting():
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Agrarian\Agrarian-Qt-testnet") as key:
                return winreg.QueryValueEx(key, "fHideZeroBalances")

        wallet_file = gui_directory / "regtest" / "wallet.dat"
        config_file = gui_directory / "agrarian.conf"
        data_hashes = (digest(wallet_file), digest(config_file))
        display_setting = wallet_setting()
        # Remove only the disposable executable from the expected destination
        # so a reinstall into the wrong folder cannot pass this test.
        old_binary = qt.with_suffix(".exe.previous")
        qt.rename(old_binary)
        install_again(0)
        assert qt.is_file() and digest(qt) == digest(old_binary)
        assert (digest(wallet_file), digest(config_file)) == data_hashes
        assert wallet_setting() == display_setting
        gui = start(qt, gui_directory, "-choosedatadir=0")
        try:
            wait_for(lambda: rpc("getwalletinfo"), gui)
            assert rpc("validateaddress", gui_address)["ismine"]
        finally:
            stop(gui)

    path = datadir("wallet")
    backup = root / "encrypted-wallet.dat"
    proc = start(daemon, path)
    try:
        wait_for(lambda: rpc("getwalletinfo"), proc)
        assert rpc("getblockchaininfo")["chain"] == "regtest"
        if installer:
            install_again(2)
            assert proc.poll() is None
        address = rpc("getnewaddress")
        assert len(rpc("generate", 101, timeout=90)) == 101
        txid = rpc("sendtoaddress", address, 1)
        rpc("generate", 1)
        assert rpc("gettransaction", txid)["confirmations"] == 1
        rpc("encryptwallet", passphrase, timeout=180)
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
    parser.add_argument("--installer", type=Path, help="Also test Windows upgrade checks")
    args = parser.parse_args()
    # The Windows candidate job starts this test from its downloaded artifact
    # directory after installing the same package into RUNNER_TEMP.
    if args.installer is None and os.name == "nt" and os.environ.get("GITHUB_JOB") == "windows-package":
        installers = list(Path.cwd().glob("*-win64-setup.exe"))
        if len(installers) != 1:
            parser.error("Expected exactly one Windows candidate installer in the artifact directory")
        args.installer = installers[0]
    with tempfile.TemporaryDirectory(prefix="agrarian-package-test-") as directory:
        test(args.qt.resolve(strict=True), args.daemon.resolve(strict=True), Path(directory),
             args.installer.resolve(strict=True) if args.installer else None)
    print("Packaged GUI launch, transaction, encryption, restart and backup restore passed")
    if args.installer:
        print("Install upgrade checks, custom folder reuse and wallet preservation passed")
