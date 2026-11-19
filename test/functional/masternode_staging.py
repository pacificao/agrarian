#!/usr/bin/env python3
"""Exercise real nominations and an enforced payout on disposable regtest.

Requires a staging-only candidate with regtest's PoW cutoff raised to at
least 1400 and the regtest spork public key set to a disposable signer.
Run only in a loopback-only namespace. Never use a production spork key.
"""

import argparse
import shutil
import socket
import tempfile
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from testnet_hybrid import Node, wait_for

ENFORCEMENT = "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT"


def run(work, daemon, spork_key):
    nodes = [Node(work / name, [str(daemon)], "regtest")
             for name in ["miner", "observer", *[f"mn{i}" for i in range(6)]]]
    miner, observer, *masternodes = nodes
    for i, node in enumerate(masternodes, start=2):
        node.p2p_host, node.p2p = f"127.0.0.{i}", 51476
    wallet_subdir = miner.log.parent
    miner.conf.write_text(miner.conf.read_text() + f"sporkkey={spork_key}\n")
    miner.conf.chmod(0o600)

    try:
        miner.start(staking=False, extra=("-par=1",))
        miner.rpc("generate", 1240, timeout=400)
        addresses = [miner.rpc("getnewaddress", raw=True) for _ in masternodes]
        keys = [miner.rpc("createmasternodekey", raw=True) for _ in masternodes]
        transactions = [miner.rpc("sendtoaddress", address, 10000, raw=True, timeout=90)
                        for address in addresses]
        miner.rpc("generate", 20, timeout=120)
        indexes = [
            next(out["n"] for out in miner.rpc("getrawtransaction", txid, 1)["vout"]
                 if out["value"] == 10000 and address in out["scriptPubKey"].get("addresses", []))
            for txid, address in zip(transactions, addresses, strict=True)
        ]

        miner.stop()
        snapshot = work / "funded-regtest.dat"
        shutil.copyfile(wallet_subdir / "blocks" / "blk00000.dat", snapshot)
        config = wallet_subdir / "masternode.conf"
        config.write_text("".join(
            f"mn{i} {node.p2p_host}:{node.p2p} {key} {txid} {index}\n"
            for i, (node, key, txid, index) in enumerate(zip(masternodes, keys, transactions, indexes, strict=True))
        ))
        config.chmod(0o600)
        miner.start(staking=False, extra=("-par=1",))

        def start_mn(node, key):
            node.conf.write_text(node.conf.read_text() + f"masternodeprivkey={key}\n")
            node.conf.chmod(0o600)
            node.start(staking=False, extra=("-par=1", "-masternode=1", f"-loadblock={snapshot}",
                                             f"-masternodeaddr={node.p2p_host}:{node.p2p}"))
            node.connect(miner)

        with ThreadPoolExecutor(max_workers=3) as workers:
            list(workers.map(start_mn, masternodes, keys))
        observer.start(staking=False, extra=("-par=1", f"-loadblock={snapshot}"))
        observer.connect(miner)
        for node in [observer, *masternodes]:
            wait_for(lambda: node.rpc("getblockcount") == 1260, timeout=180)
        # Burst mining can move fixture blocks slightly ahead of wall time.
        # Give every regtest node the same time past the collateral's maturity.
        tip = miner.rpc("getbestblockhash", raw=True)
        fixture_time = max(int(time.time()), miner.rpc("getblock", tip, "true")["time"]) + 600
        for node in nodes:
            node.rpc("setmocktime", fixture_time)
        for i in range(6):
            result = miner.rpc("startmasternode", "alias", "false", f"mn{i}")
            assert result["detail"][0]["result"] == "successful", result
        for node in nodes:
            try:
                wait_for(lambda: node.rpc("getmasternodecount")["enabled"] == 6, timeout=100)
            except AssertionError as exc:
                states = Counter(entry["status"] for entry in node.rpc("listmasternodes"))
                raise AssertionError(
                    f"{node.log.parent.parent.name} did not observe six enabled masternodes: {dict(states)}"
                ) from exc
            wait_for(lambda: node.rpc("mnsync", "status")["RequestedMasternodeAssets"] == 999,
                     timeout=75)
        print("Six collateral-backed masternodes active on both sides of staging", flush=True)

        assert miner.rpc("spork", ENFORCEMENT, 0, raw=True) == "success"
        for node in nodes:
            wait_for(lambda: node.rpc("spork", "active")[ENFORCEMENT])

        for _ in range(40):
            next_height = miner.rpc("getblockcount") + 1
            nominations = observer.rpc("getmasternodewinners", 1)
            current = next((row["winner"] for row in nominations
                            if row["nHeight"] == next_height), None)
            candidates = current if isinstance(current, list) else [current]
            quorum = next((row for row in candidates if row and row.get("nVotes", 0) >= 6), None)
            block_hash = miner.rpc("generate", 1)[0]
            wait_for(lambda: all(node.rpc("getbestblockhash", raw=True) == block_hash
                                 for node in [observer, *masternodes]))
            if quorum:
                block = observer.rpc("getblock", block_hash, "true")
                reward = observer.rpc("getrawtransaction", block["tx"][0], 1)
                assert any(out["value"] * 2 == 25
                           and quorum["address"] in out["scriptPubKey"].get("addresses", [])
                           for out in reward["vout"])
                print(f"Enforcement on: {quorum['nVotes']} votes, 12.5 test AGR paid at block {next_height}",
                      flush=True)
                return
        raise AssertionError("No enforced six-vote payee observed")
    except Exception:
        for node in nodes:
            if node.log.is_file():
                print(f"--- {node.log} ---")
                print("\n".join(node.log.read_text(errors="replace").splitlines()[-12:]))
        raise
    finally:
        for node in reversed(nodes):
            node.stop()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture-daemon", required=True, type=Path)
    parser.add_argument("--fixture-spork-key", required=True, type=Path)
    args = parser.parse_args()
    if {name for _, name in socket.if_nameindex()} != {"lo"}:
        parser.error("run this test in a loopback-only network namespace")
    with tempfile.TemporaryDirectory(prefix="agrarian-masternode-") as directory:
        run(Path(directory), args.fixture_daemon.resolve(strict=True),
            args.fixture_spork_key.read_text().strip())
