#!/usr/bin/env python3
"""Exercise the opt-in split-difficulty activation with actual local peers."""
import subprocess
import tempfile
import time
from pathlib import Path

from test_hybrid_branch_model import ProofTotals, STAGING_SCORE
from testnet_hybrid import Node, wait_for


ACTIVATION = 61
ENABLED = (f"-regtestsplitdifficultyheight={ACTIVATION}", "-par=1")


def raw_block(node, block_hash):
    return node.rpc("getblock", block_hash, "false", raw=True)


def submit_known_block(receiver, source, block_hash):
    # A stored side block may not run ConnectBlock immediately, so submitblock
    # has no BlockChecked callback and returns "inconclusive" rather than null.
    raw = raw_block(source, block_hash)
    result = receiver.rpc("submitblock", raw, raw=True)
    assert result in ("", "inconclusive"), result
    assert raw_block(receiver, block_hash) == raw


def run(directory):
    miner = Node(directory / "miner", network="regtest")
    staker = Node(directory / "staker", network="regtest")
    legacy = Node(directory / "legacy-rules", network="regtest")
    rival = Node(directory / "rival", network="regtest")
    try:
        # The wallet's MintableCoins readiness check still enforces the
        # one-hour age, even though the underlying regtest kernel skips it.
        fixture_clock = int(time.time()) - 7200
        miner.start(staking=False, extra=ENABLED)
        staker.start(staking=False, extra=ENABLED)
        legacy.start(staking=False, extra=("-par=1",))
        for node in (miner, staker, legacy):
            node.rpc("setmocktime", fixture_clock)
        staker.connect(miner)
        stake_addresses = [staker.rpc("getnewaddress", raw=True) for _ in range(2)]
        first = miner.rpc("generate", 30, timeout=90)
        for address in stake_addresses:
            # Avoid making the fixture depend on the legacy kernel's wrapped
            # multiplication at regtest's very large initial stake target.
            miner.rpc("sendtoaddress", address, 1, raw=True)
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

        # Exercise confirmation under PoW when nobody is staking.
        payment_address = miner.rpc("getnewaddress", raw=True)
        work_payment = miner.rpc("sendtoaddress", payment_address, 1, raw=True)
        work_only = miner.rpc("generate", 2)
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == work_only[-1])
        assert miner.rpc("gettransaction", work_payment)["confirmations"] == 2
        pow_tip_height = miner.rpc("getblockcount")
        work_before_stake = miner.rpc("getblocktemplate")["bits"]

        # The receiver doesn't need staking credentials. Relay this normal
        # transfer to the staker before it connects and can produce a block.
        stake_payment = miner.rpc("sendtoaddress", payment_address, 2, raw=True)
        payment_raw = miner.rpc("getrawtransaction", stake_payment, raw=True)
        staker.stop()
        staker.start(staking=True, extra=ENABLED)
        assert staker.rpc("getstakingstatus")["mintablecoins"], staker.rpc("getstakingstatus")
        staker.rpc("sendrawtransaction", payment_raw, raw=True)
        staker.connect(miner)
        wait_for(lambda: miner.rpc("getblockcount") >= pow_tip_height + 2, timeout=210)
        stake_tip = miner.rpc("getbestblockhash", raw=True)
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == stake_tip)
        for height in range(pow_tip_height + 1, pow_tip_height + 3):
            stake = miner.rpc("getblock", miner.rpc("getblockhash", height, raw=True), "true")
            assert stake["proof"] == "pos" and stake["bits"] != work_before_stake
        assert miner.rpc("gettransaction", stake_payment)["confirmations"] >= 1
        assert miner.rpc("getblocktemplate")["bits"] == work_before_stake
        following_work = miner.rpc("generate", 1)[0]
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == following_work)
        assert miner.rpc("getblock", following_work, "true")["bits"] == work_before_stake
        print("PoW-only and PoS-only each confirmed a transfer; both peers accepted PoW again after the stake-only stretch.", flush=True)

        # These competing tails use the same proof after a shared mixed
        # prefix. This exercises ordinary convergence without asserting a
        # security policy for tails that have more of opposite proofs.
        staker.stop()
        # Advance the miner's fixture clock only after the peer is gone;
        # changing it while connected would also jump socket timeout clocks.
        miner.rpc("setmocktime", int(time.time()))
        staker.start(staking=False, extra=ENABLED)
        assert staker.rpc("getbestblockhash", raw=True) == following_work
        assert not staker.rpc("getpeerinfo")
        short_tip = staker.rpc("generate", 1)[0]
        long_tip = miner.rpc("generate", 3)[-1]
        assert short_tip != long_tip
        assert staker.rpc("getbestblockhash", raw=True) == short_tip
        staker.connect(miner)
        wait_for(lambda: staker.rpc("getbestblockhash", raw=True) == long_tip, timeout=45)
        assert miner.rpc("getbestblockhash", raw=True) == long_tip
        assert staker.rpc("getblock", short_tip, "true")["confirmations"] == -1
        assert miner.rpc("gettransaction", work_payment)["confirmations"] > 0
        assert miner.rpc("gettransaction", stake_payment)["confirmations"] > 0
        print("Peers converged after competing PoW tails on a shared mixed history.", flush=True)

        # Make opposite-proof branches off the same pre-stake ancestor.
        # Follow the real proof bits, not an assumed relationship between the
        # test network's easy targets and the fixed calibration fixtures.
        rival.start(staking=False, extra=ENABLED)
        branch = miner.rpc("getblockhash", pow_tip_height, raw=True)
        for height in range(1, pow_tip_height + 1):
            block = miner.rpc("getblockhash", height, raw=True)
            assert not rival.rpc("submitblock", raw_block(miner, block), raw=True)
        assert rival.rpc("getbestblockhash", raw=True) == branch
        work_fork = rival.rpc("generate", 1)[0]
        stake_fork = [miner.rpc("getblockhash", height, raw=True)
                      for height in range(pow_tip_height + 1, pow_tip_height + 3)]
        assert all(miner.rpc("getblock", h, "true")["proof"] == "pos" for h in stake_fork)

        def branch_score(node, blocks):
            totals = ProofTotals()
            for block in blocks:
                info = node.rpc("getblock", block, "true")
                totals = totals.append(info["proof"], int(info["bits"], 16))
            return STAGING_SCORE.score(totals)

        work_score = branch_score(rival, [work_fork])
        stake_score = branch_score(miner, stake_fork)
        # In this fixture the harder fork-choice preference is PoW. The
        # native unit test separately checks the captured inverse ranking.
        assert work_score > stake_score
        for block in stake_fork:
            submit_known_block(rival, miner, block)
        assert rival.rpc("getbestblockhash", raw=True) == work_fork
        assert rival.rpc("getblock", stake_fork[-1], "true")["confirmations"] == -1
        rival.rpc("invalidateblock", work_fork)
        assert rival.rpc("getbestblockhash", raw=True) == stake_fork[-1]
        rival.rpc("reconsiderblock", work_fork)
        assert rival.rpc("getbestblockhash", raw=True) == work_fork
        print("Opposite-proof forks followed the model; invalidate/reconsider switched and restored the winner.", flush=True)

        # Produce a same-score PoW sibling on a second node. This gives two
        # nodes opposite arrival order without having to forge a block.
        tie_peer = legacy  # disposable data, containing the legacy prefix only
        tie_peer.stop()
        tie_peer.start(staking=False, extra=ENABLED)
        for height in range(ACTIVATION, pow_tip_height + 1):
            block = miner.rpc("getblockhash", height, raw=True)
            assert not tie_peer.rpc("submitblock", raw_block(miner, block), raw=True)
        sibling = tie_peer.rpc("generate", 1)[0]
        assert sibling != work_fork
        assert branch_score(tie_peer, [sibling]) == work_score
        submit_known_block(rival, tie_peer, sibling)
        submit_known_block(tie_peer, rival, work_fork)
        winner, loser = sorted((sibling, work_fork), key=lambda h: int(h, 16))
        for node in (rival, tie_peer):
            assert node.rpc("getbestblockhash", raw=True) == winner
        rival.stop()
        rival.start(staking=False, extra=ENABLED)
        assert rival.rpc("getbestblockhash", raw=True) == winner
        rival.rpc("invalidateblock", winner)
        assert rival.rpc("getbestblockhash", raw=True) == loser
        rival.rpc("reconsiderblock", winner)
        assert rival.rpc("getbestblockhash", raw=True) == winner
        print("Equal-score forks converged regardless of receipt order and retained their ranking after restart.", flush=True)
    except Exception:
        for node in (rival, staker, miner, legacy):
            if node.log.is_file():
                print(f"=== {node.log} ===")
                print("\n".join(node.log.read_text(errors="replace").splitlines()[-30:]))
        raise
    finally:
        for node in (rival, staker, miner, legacy):
            node.stop()


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="agrarian-split-difficulty-") as work:
        run(Path(work))
