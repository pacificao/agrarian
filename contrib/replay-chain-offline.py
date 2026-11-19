#!/usr/bin/env python3
"""Replay a raw Agrarian mainnet snapshot with no wallet or P2P service."""

import argparse
import base64
import hashlib
import json
import secrets
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def replay(daemon, blocks, height, tip, timeout):
    with tempfile.TemporaryDirectory(prefix="agrarian-offline-") as directory:
        datadir = Path(directory)
        password = secrets.token_hex(32)
        (datadir / "agrarian.conf").write_text(f"rpcuser=replay\nrpcpassword={password}\n")
        auth = base64.b64encode(f"replay:{password}".encode()).decode()
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]

        def rpc(method, *params):
            request = Request(
                f"http://127.0.0.1:{port}/",
                json.dumps({"id": "replay", "method": method, "params": params}).encode(),
                {"Authorization": f"Basic {auth}", "Content-Type": "application/json"},
            )
            try:
                response = urlopen(request, timeout=timeout)
            except HTTPError as error:
                response = error
            with response:
                payload = json.load(response)
            if payload["error"] is not None:
                raise RuntimeError(payload["error"])
            return payload["result"]

        command = [
            str(daemon), "-conf=" + str(datadir / "agrarian.conf"), "-datadir=" + directory,
            "-testnet=0", "-regtest=0", "-disablewallet=1", "-server=1", "-staking=0",
            "-listen=0", "-listenonion=0", "-dnsseed=0", "-dns=0", "-connect=0",
            "-discover=0", "-upnp=0", "-maxconnections=0", "-rpcbind=127.0.0.1",
            f"-rpcport={port}", *[f"-loadblock={path}" for path in blocks],
        ]
        with subprocess.Popen(command) as process:
            try:
                deadline = time.monotonic() + timeout
                info = None
                while time.monotonic() < deadline and process.poll() is None:
                    try:
                        info = rpc("getblockchaininfo")
                        if info["blocks"] >= height:
                            break
                    except (OSError, URLError, RuntimeError, ValueError):
                        pass  # RPC starts before import finishes.
                    time.sleep(0.5)

                if not info or (info["chain"], info["blocks"], info["bestblockhash"]) != (
                        "main", height, tip):
                    raise RuntimeError(f"Snapshot did not reach the expected mainnet tip: {info}")
                # Agrarian's verifychain takes only depth; it uses level 4 internally.
                if rpc("getconnectioncount") != 0 or not rpc("verifychain", 0):
                    raise RuntimeError("Offline or full chain verification check failed")
                utxo = rpc("gettxoutsetinfo")
                if utxo["height"] != height or utxo["bestblock"] != tip:
                    raise RuntimeError("UTXO verification did not run at the snapshot tip")
                samples = []
                for sample in sorted({0, 1, 2, 3, height // 2, max(0, height - 1), height}):
                    if sample <= height:
                        block = rpc("getblock", rpc("getblockhash", sample), True)
                        samples.append({key: block[key] for key in ("height", "hash", "modifier")})

                return {
                    "chain": info["chain"], "height": height, "tip": tip,
                    "utxo": {key: utxo[key] for key in (
                        "hash_serialized", "transactions", "txouts", "total_amount",
                    )},
                    "sample_blocks": samples,
                }
            except Exception:
                log = datadir / "debug.log"
                if log.is_file():
                    print("\n".join(log.read_text(errors="replace").splitlines()[-30:]), file=sys.stderr)
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=10)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--daemon", type=Path, required=True)
    parser.add_argument("--height", type=int, required=True)
    parser.add_argument("--tip", required=True)
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("blocks", type=Path, nargs="+")
    args = parser.parse_args()
    paths = [path.resolve(strict=True) for path in args.blocks]
    result = replay(args.daemon.resolve(strict=True), paths, args.height, args.tip, args.timeout)
    result["inputs"] = [
        {"name": path.name, "sha256": digest(path)} for path in paths
    ]
    print(json.dumps(result, indent=2, sort_keys=True))


def digest(path):
    with path.open("rb") as handle:
        return hashlib.file_digest(handle, "sha256").hexdigest()


if __name__ == "__main__":
    main()
