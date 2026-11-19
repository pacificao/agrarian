#!/usr/bin/env python3
"""Exercise real nominations and an enforced payout on disposable regtest.

Requires a staging-only candidate with regtest's PoW cutoff raised to at
least 1400 and the regtest spork public key set to a disposable signer.
The fixture can schedule pings more frequently on regtest; it must not
shorten the protocol's required ten-minute activation interval.
Run only in a loopback-only namespace. Never use a production spork key.
"""

import argparse
import shutil
import socket
import tempfile
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from decimal import Decimal
from pathlib import Path

from testnet_hybrid import Node, wait_for

ENFORCEMENT = "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT"
UPDATED_NODES = "SPORK_10_MASTERNODE_PAY_UPDATED_NODES"


def run(work, daemon, spork_key):
    nodes = [Node(work / name, [str(daemon)], "regtest")
             for name in ["miner", "observer", "staker", *[f"mn{i}" for i in range(6)]]]
    miner, observer, staker, *masternodes = nodes
    for i, node in enumerate(masternodes, start=2):
        node.p2p_host, node.p2p = f"127.0.0.{i}", 51476
    wallet_subdir = miner.log.parent
    miner.conf.write_text(miner.conf.read_text() + f"sporkkey={spork_key}\n")
    miner.conf.chmod(0o600)

    try:
        miner.start(staking=False, extra=("-par=1",))
        staker.start(staking=False, extra=("-par=1",))
        stake_address = staker.rpc("getnewaddress", raw=True)
        staker.stop()
        miner.rpc("generate", 1240, timeout=400)
        addresses = [miner.rpc("getnewaddress", raw=True) for _ in masternodes]
        keys = [miner.rpc("createmasternodekey", raw=True) for _ in masternodes]
        transactions = [miner.rpc("sendtoaddress", address, 10000, raw=True, timeout=90)
                        for address in addresses]
        miner.rpc("sendtoaddress", stake_address, 50, raw=True)
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
        # Run the staking thread but reserve the only input until its payee
        # has quorum. This avoids restarting against a future mock-time tip.
        staker.start(staking=True, extra=("-par=1", f"-loadblock={snapshot}"))
        assert staker.rpc("reservebalance", "true", 50)["reserve"]
        staker.connect(miner)
        for node in nodes:
            wait_for(lambda: node.rpc("getblockcount") == 1260, timeout=180)
        for node in nodes:
            wait_for(lambda: node.rpc("mnsync", "status")["RequestedMasternodeAssets"] == 999,
                     timeout=180)
        # Use the modern broadcast and signed-ping path, not legacy relays.
        assert miner.rpc("spork", UPDATED_NODES, 0, raw=True) == "success"
        for node in nodes:
            wait_for(lambda: node.rpc("spork", "active")[UPDATED_NODES])
        # Burst mining can move fixture blocks slightly ahead of wall time.
        # Give every regtest node the same time past the collateral's maturity.
        tip = miner.rpc("getbestblockhash", raw=True)
        fixture_time = max(int(time.time()), miner.rpc("getblock", tip, "true")["time"]) + 600
        for node in nodes:
            node.rpc("setmocktime", fixture_time)
        result = miner.rpc("startmasternode", "all", "false")
        assert len(result["detail"]) == 6 and all(
            row["result"] == "success" for row in result["detail"]
        ), result
        for node in nodes:
            wait_for(lambda: node.rpc("getmasternodecount")["total"] == 6)
        # Let the periodic state check refresh without advancing far enough
        # for the first activation ping.
        for node in nodes:
            node.rpc("setmocktime", fixture_time + 10)
        # The legacy RPC calls the internal PRE_ENABLED state "ACTIVE".
        observed = observer.rpc("listmasternodes")
        assert len(observed) == 6 and all(
            entry["status"] == "ACTIVE" and entry["rank"] == 0 and entry["activetime"] == 0
            for entry in observed
        ), f"Unexpected pre-activation states: {dict(Counter(entry['status'] for entry in observed))}"
        assert observer.rpc("getmasternodecount")["enabled"] == 0
        print("Bulk registration relayed all six masternodes; observer saw six pending", flush=True)
        for node in nodes:
            node.rpc("setmocktime", fixture_time + 601)
        wait_for(lambda: observer.rpc("getmasternodecount")["enabled"] == 6)
        # Local sends update the ping immediately, but the hot node's own
        # enabled state is refreshed on its next periodic Check().
        for node in nodes:
            node.rpc("setmocktime", fixture_time + 611)
        for node in nodes:
            try:
                wait_for(lambda: node.rpc("getmasternodecount")["enabled"] == 6, timeout=100)
            except AssertionError as exc:
                states = Counter(entry["status"] for entry in node.rpc("listmasternodes"))
                raise AssertionError(
                    f"{node.log.parent.parent.name} did not observe six enabled masternodes: {dict(states)}"
                ) from exc
        print("Six collateral-backed masternodes active on both sides of staging", flush=True)

        # The enforcement spork also checks a distinct 8,000-second voting age.
        # Keep mock-time steps below the 20-minute socket inactivity timeout.
        # A fresh ping at each step also keeps the collateral entries live.
        for offset in range(1551, 8202, 950):
            now = fixture_time + offset
            for node in nodes:
                node.rpc("setmocktime", now)
            def all_pinged():
                entries = observer.rpc("listmasternodes")
                return len(entries) == 6 and all(
                    entry["status"] == "ENABLED" and entry["lastseen"] >= now for entry in entries
                )
            wait_for(all_pinged)

        assert miner.rpc("spork", ENFORCEMENT, 0, raw=True) == "success"
        for node in nodes:
            wait_for(lambda: node.rpc("spork", "active")[ENFORCEMENT])

        def voted_payee(height):
            nominations = observer.rpc("getmasternodewinners", 1)
            current = next((row["winner"] for row in nominations
                            if row["nHeight"] == height), None)
            candidates = current if isinstance(current, list) else [current]
            return next((row for row in candidates if row and row.get("nVotes", 0) >= 6), None)

        def mine():
            block_hash = miner.rpc("generate", 1)[0]
            wait_for(lambda: all(node.rpc("getbestblockhash", raw=True) == block_hash
                                 for node in nodes))
            return block_hash

        def payout(block_hash, index, quorum, value):
            block = observer.rpc("getblock", block_hash, "true")
            reward = observer.rpc("getrawtransaction", block["tx"][index], 1)
            assert any(out["value"] == value
                       and quorum["address"] in out["scriptPubKey"].get("addresses", [])
                       for out in reward["vout"])
            return reward

        for _ in range(40):
            next_height = miner.rpc("getblockcount") + 1
            quorum = voted_payee(next_height)
            block_hash = mine()
            if quorum:
                payout(block_hash, 0, quorum, Decimal("12.5"))
                print(f"Enforced PoW: {quorum['nVotes']} votes, 12.5 test AGR paid at block {next_height}",
                      flush=True)
                break
        else:
            raise AssertionError("No enforced six-vote PoW payee observed")

        for _ in range(20):
            next_height = miner.rpc("getblockcount") + 1
            quorum = voted_payee(next_height)
            if quorum:
                break
            mine()
        else:
            raise AssertionError("No six-vote payee for the next staking block")
        assert staker.rpc("reservebalance")["reserve"]
        assert not staker.rpc("reservebalance", "false")["reserve"]
        for extra_time in range(15, 181, 15):
            staker.rpc("setmocktime", fixture_time + 8201 + extra_time)
            try:
                wait_for(lambda: observer.rpc("getblockcount") >= next_height, timeout=8)
                break
            except AssertionError:
                continue
        else:
            raise AssertionError("Staker did not produce the nominated block")
        assert observer.rpc("getblockcount") == next_height
        block_hash = observer.rpc("getbestblockhash", raw=True)
        wait_for(lambda: all(node.rpc("getbestblockhash", raw=True) == block_hash for node in nodes))
        assert observer.rpc("getblock", block_hash, "true")["proof"] == "pos"
        reward = payout(block_hash, 1, quorum, Decimal("2.5"))
        stake_input = sum(
            observer.rpc("getrawtransaction", txin["txid"], 1)["vout"][txin["vout"]]["value"]
            for txin in reward["vin"]
        )
        assert sum(out["value"] for out in reward["vout"]) - stake_input == 10
        print(f"Enforced PoS: {quorum['nVotes']} votes, 2.5 of 10 test AGR paid at block {next_height}",
              flush=True)
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
