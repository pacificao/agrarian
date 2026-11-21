#!/usr/bin/env python3
"""Wallet-free boundary rehearsal using a disposable mainnet-profile build.

The shadow binary must activate at snapshot height + 1; do not publish it.
The legacy binary must not carry that profile. Run both inside the same
loopback-only namespace. Never pass actual node data directories or wallets.
"""
import argparse
import hashlib
import json
import socket
import struct
import subprocess
import tempfile
import time
from pathlib import Path

from test_hybrid_branch_model import decode_target
from testnet_hybrid import COIN, Node, build_solver, sha256d, wait_for


WORK_BITS = "1d216ade"
STAKE_BITS = "1c010000"


class ShadowNode(Node):
    def __init__(self, directory, daemon):
        super().__init__(directory, daemon=[str(daemon)], network="main")
        self.common.remove("-main")  # mainnet is the default, not an option.
        self.log = directory / "debug.log"

    def ready(self):
        if self.proc.poll() is not None:
            raise RuntimeError("shadow daemon exited")
        try:
            return self.rpc("getblockchaininfo")["chain"] == "main"
        except subprocess.CalledProcessError:
            return False

    def start_shadow(self, snapshot):
        self.proc = subprocess.Popen([
            *self.daemon, *self.common, "-disablewallet=1", "-staking=0", "-server=1",
            "-listen=0", "-listenonion=0", "-connect=0", "-dnsseed=0", "-dns=0",
            "-discover=0", "-upnp=0", "-maxconnections=0", "-rpcbind=127.0.0.1",
            "-txindex=1", "-par=1", f"-loadblock={snapshot}",
        ])
        wait_for(self.ready, timeout=180)


def make_pow_block(parent, height, timestamp, checkpoint, solver):
    number = height.to_bytes((height.bit_length() + 7) // 8, "little")
    if number[-1] & 0x80:
        number += b"\0"
    script = bytes([len(number)]) + number
    coinbase = (struct.pack("<I", 1) + b"\x01" + bytes(32) + b"\xff" * 4
                + bytes([len(script)]) + script + b"\xff" * 4 + b"\x01"
                + struct.pack("<Q", 50 * COIN) + b"\x01\x51" + bytes(4))
    header = (struct.pack("<I", 5) + bytes.fromhex(parent)[::-1] + sha256d(coinbase)
              + struct.pack("<III", timestamp, int(WORK_BITS, 16), 0))
    target = decode_target(int(WORK_BITS, 16)).to_bytes(32, "big")
    result = subprocess.run([str(solver)], input=header + target, check=True,
                            capture_output=True, timeout=5400)
    return header[:76] + struct.pack("<I", int(result.stdout)) + checkpoint + b"\x01" + coinbase


def run(args, directory):
    with args.snapshot.open("rb") as snapshot:
        actual_sha = hashlib.file_digest(snapshot, "sha256").hexdigest()
    ref = json.loads(args.replay.read_text())
    assert actual_sha == ref["inputs"][0]["sha256"] and ref["chain"] == "main"
    height, tip = ref["height"], ref["tip"]
    challenger = ShadowNode(directory / "candidate", args.candidate)
    baseline = ShadowNode(directory / "legacy", args.legacy)
    try:
        for node in (challenger, baseline):
            node.start_shadow(args.snapshot)
        for node in (challenger, baseline):
            wait_for(lambda: node.rpc("getblockcount") >= height, timeout=900)
            assert node.rpc("getbestblockhash", raw=True) == tip
            assert node.rpc("gettxoutsetinfo")["hash_serialized"] == ref["utxo"]["hash_serialized"]
            assert node.rpc("getconnectioncount") == 0
        fork = challenger.rpc("getblockchaininfo")["hybrid"]
        assert "hybrid" not in baseline.rpc("getblockchaininfo")
        assert fork == {"activation_height": height + 1, "split_targets_for_next_block": True,
                        "normalized_fork_choice_at_tip": False, "next_pow_bits": WORK_BITS,
                        "next_pos_bits": STAKE_BITS}, fork
        print(f"Both daemons replayed mainnet through {height}; shadow reports the expected two starting targets.", flush=True)

        solver = directory / "quark"
        build_solver(solver)
        anchor = challenger.rpc("getblock", tip, "true")
        raw = bytes.fromhex(challenger.rpc("getblock", tip, "false", raw=True))
        checkpoint = raw[80:112] if anchor["version"] > 3 else bytes(32)
        print(f"Solving wallet-free shadow work at height {height + 1}; no network interfaces are available.", flush=True)
        block = make_pow_block(tip, height + 1, max(int(time.time()), anchor["time"] + 1), checkpoint, solver)
        assert not challenger.rpc("submitblock", block.hex(), raw=True)
        try:
            baseline.rpc("submitblock", block.hex(), raw=True)
        except subprocess.CalledProcessError:
            pass
        assert baseline.rpc("getbestblockhash", raw=True) == tip
        assert challenger.rpc("getblockcount") == height + 1
        after = challenger.rpc("getblockchaininfo")["hybrid"]
        assert after["normalized_fork_choice_at_tip"] and after["next_pos_bits"] == STAKE_BITS
        print("Shadow node accepted the activation block, legacy stayed at the parent, and the first PoS target remained unchanged.", flush=True)
    except Exception:
        for node in (challenger, baseline):
            if node.log.is_file():
                print("\n".join(node.log.read_text(errors="replace").splitlines()[-20:]))
        raise
    finally:
        baseline.stop()
        challenger.stop()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for param in ("snapshot", "replay", "candidate", "legacy"):
        parser.add_argument("--" + param, required=True, type=lambda x: Path(x).resolve(strict=True))
    args = parser.parse_args()
    if {name for _, name in socket.if_nameindex()} != {"lo"}:
        parser.error("shadow rehearsal requires a loopback-only network namespace")
    with tempfile.TemporaryDirectory(prefix="agrarian-mainnet-shadow-") as name:
        run(args, Path(name))
