#!/usr/bin/env python3
"""Exercise real testnet rules with two loopback-only nodes and fresh wallets.

Build agrariand and agrarian-cli first. Run this script directly (not via the
regtest runner). Historical PoW timestamps let the fixture mature stake without
waiting an hour or changing testnet parameters.
"""
import hashlib
import json
import secrets
import socket
import struct
import subprocess
import tempfile
import time
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COIN = 100_000_000
TESTNET_POW_LIMIT = 0x1E0FFFFF


def wait_for(fn, timeout=90):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = fn()
        if result:
            return result
        time.sleep(0.25)
    raise AssertionError("Timed out waiting for " + fn.__name__)


class Node:
    def __init__(self, directory):
        directory.mkdir()
        with socket.socket() as p2p, socket.socket() as rpc:
            p2p.bind(("127.0.0.1", 0))
            rpc.bind(("127.0.0.1", 0))
            self.p2p = p2p.getsockname()[1]
            rpc_port = rpc.getsockname()[1]
        self.common = ["-testnet", f"-datadir={directory}", f"-rpcport={rpc_port}"]
        self.conf = directory / "agrarian.conf"
        self.conf.write_text(f"rpcuser=testnet\nrpcpassword={secrets.token_hex(24)}\n")
        self.log = directory / "testnet4" / "debug.log"

    def start(self, staking):
        self.proc = subprocess.Popen([
            str(ROOT / "src/agrariand"), *self.common,
            "-server=1", "-txindex=1", "-keypool=5", "-listen=1", "-bind=127.0.0.1",
            "-rpcbind=127.0.0.1", f"-port={self.p2p}", "-connect=0",
            "-dnsseed=0", "-discover=0", "-upnp=0", "-listenonion=0",
            f"-staking={int(staking)}", "-debug=staking", "-debug=masternode",
        ])
        wait_for(self.ready, timeout=30)
        assert self.rpc("getblockchaininfo")["chain"] == "test"

    def ready(self):
        try:
            return self.rpc("getblockcount") >= 0
        except subprocess.CalledProcessError:
            if self.proc.poll() is not None:
                raise
            return False

    def rpc(self, method, *args, raw=False):
        completed = subprocess.run([
            str(ROOT / "src/agrarian-cli"), *self.common, method, *map(str, args),
        ], capture_output=True, text=True, timeout=15)
        if completed.returncode:
            raise subprocess.CalledProcessError(completed.returncode, method, stderr=completed.stderr)
        result = completed.stdout.strip()
        if raw or not result:
            return result
        return json.loads(result, parse_float=Decimal)

    def connect(self, other):
        self.rpc("addnode", f"127.0.0.1:{other.p2p}", "onetry")
        wait_for(lambda: self.rpc("getconnectioncount") == 1)

    def stop(self):
        if not hasattr(self, "proc") or self.proc.poll() is not None:
            return
        self.proc.terminate()
        try:
            self.proc.wait(timeout=30)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait(timeout=10)

    def pubkey(self):
        address = self.rpc("getnewaddress", raw=True)
        return bytes.fromhex(self.rpc("validateaddress", address)["pubkey"])


def sha256d(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


def mine_pow(node, helper, height, timestamp, pubkey, template=None):
    """Mine a one-transaction block below the first difficulty retarget."""
    assert height <= 24
    previous_hash = node.rpc("getbestblockhash", raw=True)
    prev = bytes.fromhex(previous_hash)[::-1]
    version, bits, reward, checkpoint = 5, TESTNET_POW_LIMIT, 50 * COIN, bytes(32)
    # CScript's height encoding followed by OP_0 for minimum coinbase script size.
    sig = bytes([0x50 + height, 0]) if height <= 16 else bytes([1, height, 0])
    if template is not None:
        assert template["previousblockhash"] == previous_hash and template["height"] == height
        assert template["transactions"] == [] and template["coinbasevalue"] == reward
        version, bits = template["version"], int(template["bits"], 16)
        checkpoint = bytes.fromhex(template["acc_checkpoint"])[::-1]
        sig += bytes.fromhex(template["coinbaseaux"]["flags"])
        timestamp = max(timestamp, template["mintime"], template["curtime"])

    script = bytes([len(pubkey)]) + pubkey + b"\xac"
    coinbase = (struct.pack("<I", 1) + b"\x01" + bytes(32) + bytes([255]) * 4
                + bytes([len(sig)]) + sig + bytes([255]) * 4 + b"\x01"
                + struct.pack("<Q", reward) + bytes([len(script)]) + script + bytes(4))
    header = struct.pack("<I", version) + prev + sha256d(coinbase) + struct.pack(
        "<III", timestamp, bits, 0,
    )
    target = (bits & 0xffffff) << (8 * ((bits >> 24) - 3))
    if template is not None:
        assert target == int(template["target"], 16)
    nonce = subprocess.run([str(helper)], input=header + target.to_bytes(32, "big"),
                           capture_output=True, check=True, timeout=240).stdout
    block = header[:76] + struct.pack("<I", int(nonce)) + checkpoint + b"\x01" + coinbase
    result = node.rpc("submitblock", block.hex(), raw=True)
    assert not result, f"PoW rejected at {height}: {result}"
    assert node.rpc("getblockcount") == height
    tip = node.rpc("getbestblockhash", raw=True)
    assert node.rpc("getblock", tip, "true")["proof"] == "pow"
    return tip


def run(work):
    helper = work / "testnet-hybrid-pow"
    subprocess.run([
        "c++", "-std=c++17", "-O2", "-pthread", "-I", str(ROOT / "src"),
        str(Path(__file__).with_name("testnet_hybrid_pow.cpp")),
        str(ROOT / "src/crypto/libbitcoin_crypto.a"), "-o", str(helper),
    ], check=True)
    staker, verifier = Node(work / "staker"), Node(work / "verifier")
    try:
        staker.start(staking=False)
        verifier.start(staking=False)
        verifier.connect(staker)
        stake_key, other_key = staker.pubkey(), verifier.pubkey()
        now = int(time.time())
        for height in range(1, 21):
            # Only block 2 pays the staker, so there is exactly one mature staking input.
            stamp = now - 18_000 + height * 600 if height < 20 else now - 10
            tip = mine_pow(staker, helper, height, stamp, stake_key if height == 2 else other_key)
            wait_for(lambda: verifier.rpc("getbestblockhash", raw=True) == tip)
            modifier = staker.rpc("getblock", tip, "true")["modifier"]
            assert modifier == verifier.rpc("getblock", tip, "true")["modifier"], height
            if height == 3:
                assert modifier == "7374616b656d6f64"
        print("Two testnet nodes accepted and relayed 20 real PoW blocks", flush=True)

        staker.stop()
        staker.start(staking=True)
        staker.connect(verifier)
        wait_for(lambda: verifier.rpc("getblockcount") == 21, timeout=240)
        tip = verifier.rpc("getbestblockhash", raw=True)
        assert staker.rpc("getbestblockhash", raw=True) == tip
        block = verifier.rpc("getblock", tip, "true")
        assert block["height"] == 21 and block["proof"] == "pos" and len(block["tx"]) == 2
        stake = verifier.rpc("getrawtransaction", block["tx"][1], 1)
        assert stake["vout"][0]["value"] == 0 and stake["vout"][0]["scriptPubKey"]["hex"] == ""
        source = sum(
            verifier.rpc("getrawtransaction", txin["txid"], 1)["vout"][txin["vout"]]["value"]
            for txin in stake["vin"]
        )
        assert sum(out["value"] for out in stake["vout"]) - source == 10
        print(f"Both testnet nodes accepted signed PoS block 21 with a 10 AGR subsidy: {tip}", flush=True)

        template = verifier.rpc("getblocktemplate")
        post_pos = mine_pow(verifier, helper, 22, block["time"] + 1, other_key, template)
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == post_pos)
        print(f"Both nodes accepted PoW block 22 from getblocktemplate after PoS: {post_pos}", flush=True)
    except Exception:
        for node in (staker, verifier):
            if node.log.exists():
                print(f"=== {node.log} ===")
                print("\n".join(node.log.read_text(errors="replace").splitlines()[-45:]))
        raise
    finally:
        try:
            staker.stop()
        finally:
            verifier.stop()


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="agrarian-testnet-hybrid-") as directory:
        run(Path(directory))
