#!/usr/bin/env python3
"""Exercise the opt-in split-difficulty activation with actual local peers."""
import subprocess
import tempfile
import time
from pathlib import Path

from testnet_hybrid import Node, wait_for


ACTIVATION = 61
ENABLED = (f"-regtestsplitdifficultyheight={ACTIVATION}", "-par=1")


def raw_block(node, block_hash):
    return node.rpc("getblock", block_hash, "false", raw=True)


def run(directory):
    miner = Node(directory / "miner", network="regtest")
    staker = Node(directory / "staker", network="regtest")
    legacy = Node(directory / "legacy-rules", network="regtest")
    try:
        # The wallet's MintableCoins readiness check still enforces the
        # one-hour age, even though the underlying regtest kernel skips it.
        fixture_clock = f"-mocktime={int(time.time()) - 7200}"
        miner.start(staking=False, extra=(*ENABLED, fixture_clock))
        staker.start(staking=False, extra=(*ENABLED, fixture_clock))
        legacy.start(staking=False, extra=("-par=1", fixture_clock))
        staker.connect(miner)
        stake_address = staker.rpc("getnewaddress", raw=True)
        first = miner.rpc("generate", 30, timeout=90)
        miner.rpc("sendtoaddress", stake_address, 50, raw=True)
        history = first + miner.rpc("generate", 30, timeout=90)
        assert len(history) == ACTIVATION - 1
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == history[-1])
        for block in history:
            assert not legacy.rpc("submitblock", raw_block(miner, block), raw=True)
        assert legacy.rpc("getbestblockhash", raw=True) == history[-1]

        old_bits = legacy.rpc("getblock", history[-1], "true")["bits"]
        template = miner.rpc("getblocktemplate")
        assert template["height"] == ACTIVATION and template["bits"] != old_bits
        activation_block = miner.rpc("generate", 1)[0]
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == activation_block)
        assert miner.rpc("getblock", activation_block, "true")["bits"] == template["bits"]
        try:
            legacy.rpc("submitblock", raw_block(miner, activation_block), raw=True)
        except subprocess.CalledProcessError:
            pass  # Legacy regtest has a different, fixed expected target.
        assert legacy.rpc("getblockcount") == ACTIVATION - 1
        print("Legacy and candidate agreed before activation; candidate peers accepted the new PoW target.", flush=True)

        work_before_stake = miner.rpc("getblocktemplate")["bits"]
        staker.stop()
        staker.start(staking=True, extra=ENABLED)
        staker.connect(miner)
        wait_for(lambda: miner.rpc("getblockcount") == ACTIVATION + 1, timeout=180)
        stake_tip = miner.rpc("getbestblockhash", raw=True)
        assert staker.rpc("getbestblockhash", raw=True) == stake_tip
        stake = miner.rpc("getblock", stake_tip, "true")
        assert stake["proof"] == "pos" and stake["bits"] != work_before_stake
        assert miner.rpc("getblocktemplate")["bits"] == work_before_stake
        following_work = miner.rpc("generate", 1)[0]
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == following_work)
        assert miner.rpc("getblock", following_work, "true")["bits"] == work_before_stake
        print("Both peers accepted signed PoS followed by PoW; stake did not alter the PoW template.", flush=True)
    except Exception:
        for node in (staker, miner, legacy):
            if node.log.is_file():
                print(f"=== {node.log} ===")
                print("\n".join(node.log.read_text(errors="replace").splitlines()[-30:]))
        raise
    finally:
        for node in (staker, miner, legacy):
            node.stop()


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="agrarian-split-difficulty-") as work:
        run(Path(work))
