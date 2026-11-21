// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "coins.h"
#include "kernel.h"
#include "main.h"
#include "masternode-payments.h"
#include "pow.h"
#include "script/standard.h"
#include "test_agrarian.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(hybrid_block_subsidy)
{
    const int firstStake = Params().FIRST_POS_BLOCK();
    const int lastWork = Params().LAST_POW_BLOCK();
    BOOST_REQUIRE_LT(firstStake, lastWork);

    // The genesis return value is historical; genesis itself bypasses
    // ordinary subsidy validation. Test the spendable-block schedule below.
    BOOST_CHECK_EQUAL(GetBlockValue(1, false), 50 * COIN);
    BOOST_CHECK_EQUAL(GetBlockValue(firstStake, false), 50 * COIN);
    BOOST_CHECK_EQUAL(GetBlockValue(firstStake, true), 10 * COIN);
    BOOST_CHECK_EQUAL(GetBlockValue(lastWork, false), 50 * COIN);
    BOOST_CHECK_EQUAL(GetBlockValue(lastWork, true), 10 * COIN);
    BOOST_CHECK_EQUAL(GetBlockValue(lastWork + 1, false), 0);
    BOOST_CHECK_EQUAL(GetBlockValue(lastWork + 1, true), 10 * COIN);
}

BOOST_AUTO_TEST_CASE(hybrid_masternode_payee_uses_block_proof)
{
    const CScript payee = GetScriptForDestination(CKeyID(uint160(1)));
    CMasternodeBlockPayees payees(Params().FIRST_POS_BLOCK() + 20);
    payees.AddPayee(payee, MNPAYMENTS_SIGNATURES_REQUIRED);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vout.emplace_back(50 * COIN - 1250 * CENT, CScript() << OP_TRUE);
    coinbase.vout.emplace_back(1250 * CENT, payee);
    BOOST_CHECK(payees.IsTransactionValid(CTransaction(coinbase)));
    --coinbase.vout.back().nValue;
    BOOST_CHECK(!payees.IsTransactionValid(CTransaction(coinbase)));

    CMutableTransaction coinstake;
    coinstake.vin.emplace_back(COutPoint(uint256(1), 0));
    coinstake.vout.resize(1);
    coinstake.vout[0].SetEmpty();
    coinstake.vout.emplace_back(50 * COIN + 10 * COIN - 250 * CENT, CScript() << OP_TRUE);
    coinstake.vout.emplace_back(250 * CENT, payee);
    BOOST_REQUIRE(CTransaction(coinstake).IsCoinStake());
    BOOST_CHECK(payees.IsTransactionValid(CTransaction(coinstake)));
    --coinstake.vout.back().nValue;
    BOOST_CHECK(!payees.IsTransactionValid(CTransaction(coinstake)));
}

BOOST_AUTO_TEST_CASE(split_difficulty_isolates_stake_bursts)
{
    std::vector<CBlockIndex> history(124);
    for (size_t i = 1; i < history.size(); ++i) {
        history[i].nHeight = i;
        history[i].pprev = &history[i - 1];
        history[i].nTime = 1000000 + i * (2 * Params().TargetSpacing());
        history[i].nBits = 0x1d2f75d0;
    }
    const auto work = GetNextSplitWorkRequired(&history[24], BlockProof::Work);
    BOOST_CHECK_EQUAL(work, 0x1d2f75d0u);
    const uint256& fixtureLimit = Params().ProofOfWorkLimit();
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history[24], BlockProof::Stake, fixtureLimit),
                      Params().ProofOfWorkLimit().GetCompact());

    // The observed mainnet incident comprised 99 stake blocks over 155 s.
    for (size_t i = 25; i < history.size(); ++i) {
        history[i].SetProofOfStake();
        history[i].nTime = history[24].nTime + (i - 25) * 155 / 98;
        history[i].nBits = 0x1c04fd77;
    }
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history.back(), BlockProof::Work), work);
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history[25], BlockProof::Stake, fixtureLimit), 0x1c04fd77u);
    uint256 prior, next;
    prior.SetCompact(history.back().nBits);
    next.SetCompact(GetNextSplitWorkRequired(&history.back(), BlockProof::Stake, fixtureLimit));
    BOOST_CHECK_EQUAL(next.GetCompact(), uint256(prior / uint256(3)).GetCompact());
    BOOST_CHECK_NE(GetNextWorkRequired(&history.back(), BlockProof::Work), work);
    BOOST_CHECK(!IsSplitDifficultyActive(&history.back()));
}

BOOST_AUTO_TEST_CASE(split_difficulty_never_exceeds_the_target_limit)
{
    struct Restore {
        CBaseChainParams::Network network = Params().NetworkID();
        ~Restore() { SelectParams(network); }
    } restore;
    SelectParams(CBaseChainParams::REGTEST);
    const unsigned int limit = Params().ProofOfWorkLimit().GetCompact();
    std::vector<CBlockIndex> history(25);
    for (size_t i = 1; i < history.size(); ++i) {
        history[i].nHeight = i;
        history[i].pprev = &history[i - 1];
        history[i].nTime = 1000000 + i * 100000;
        history[i].nBits = limit;
    }
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history.back(), BlockProof::Work), limit);
}

BOOST_AUTO_TEST_CASE(split_difficulty_respects_an_independent_stake_bootstrap_bound)
{
    uint256 stakeLimit;
    stakeLimit.SetCompact(0x1c04bc95); // staging fixture, not an approved mainnet bound
    const uint256& workLimit = Params().ProofOfWorkLimit();
    BOOST_REQUIRE(stakeLimit < workLimit);
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(nullptr, BlockProof::Stake, stakeLimit), stakeLimit.GetCompact());
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(nullptr, BlockProof::Work, workLimit), workLimit.GetCompact());

    CBlockIndex genesis, oldStake, newest;
    oldStake.pprev = &genesis;
    oldStake.nHeight = 1;
    oldStake.nTime = 1000;
    oldStake.nBits = workLimit.GetCompact();
    oldStake.SetProofOfStake();
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&oldStake, BlockProof::Stake, stakeLimit), stakeLimit.GetCompact());

    newest.pprev = &oldStake;
    newest.nHeight = 2;
    newest.nTime = oldStake.nTime + 2 * Params().TargetSpacing();
    newest.nBits = 0x1c023000;
    newest.SetProofOfStake();
    uint256 newestTarget;
    newestTarget.SetCompact(newest.nBits);
    // The easier legacy sample contributes timing, but its target is capped.
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&newest, BlockProof::Stake, stakeLimit),
                      uint256((stakeLimit + newestTarget) / uint256(2)).GetCompact());
    BOOST_CHECK(Params().SplitStakeLimit() == 0);
    BOOST_CHECK_EQUAL(HybridConsensusHeight(), 0);
    BOOST_CHECK(!IsSplitDifficultyActive(&newest));
}

BOOST_AUTO_TEST_CASE(public_hybrid_activation_requires_a_complete_profile)
{
    struct FixtureParams : CChainParams {
        FixtureParams() {
            networkID = CBaseChainParams::MAIN;
            nFirstPoSBlock = 2;
            nLastPOWBlock = 20000;
            bnProofOfWorkLimit.SetCompact(0x1e0fffff);
        }
        const Checkpoints::CCheckpointData& Checkpoints() const override { return Params().Checkpoints(); }
        void Height(int h) { nSplitDifficultyHeight = h; }
        void Bound(const uint256& limit) { bnSplitStakeLimit = limit; }
        void WorkBootstrap(unsigned int bits) { nSplitWorkBootstrapBits = bits; }
    } fixture;

    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    fixture.Height(10000);
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    uint256 limit;
    limit.SetCompact(0x1c010000);
    fixture.Bound(limit);
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    fixture.WorkBootstrap(0x1c0dd754);
    fixture.Bound(limit + 1); // Rounded down by the wire's compact representation.
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    fixture.Bound(fixture.ProofOfWorkLimit());
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    fixture.Bound(limit);
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 10000);
    fixture.WorkBootstrap(0x1e8fffff); // Negative compact target.
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
    fixture.WorkBootstrap(0x1c0dd754);
    fixture.Height(20001);
    BOOST_CHECK_EQUAL(PublicHybridConsensusHeight(fixture), 0);
}

BOOST_AUTO_TEST_CASE(public_hybrid_bootstrap_ignores_legacy_same_proof_history)
{
    constexpr int first = 10000;
    constexpr unsigned int workBits = 0x1c0dd754;
    constexpr unsigned int stakeBits = 0x1c010000;
    uint256 workLimit, stakeLimit;
    workLimit.SetCompact(0x1e0fffff);
    stakeLimit.SetCompact(stakeBits);
    CBlockIndex legacyWork, legacyStake, newWork;
    legacyWork.nHeight = first - 2;
    legacyWork.nBits = 0x1d173c32;
    legacyStake.nHeight = first - 1;
    legacyStake.nBits = 0x1c04bc95;
    legacyStake.pprev = &legacyWork;
    legacyStake.SetProofOfStake();
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&legacyStake, BlockProof::Work, workLimit, first, workBits), workBits);
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&legacyStake, BlockProof::Stake, stakeLimit, first, stakeBits), stakeBits);

    newWork.nHeight = first;
    newWork.nBits = workBits;
    newWork.pprev = &legacyStake;
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&newWork, BlockProof::Stake, stakeLimit, first, stakeBits), stakeBits);
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&newWork, BlockProof::Work, workLimit, first, workBits), workBits);
}

BOOST_AUTO_TEST_CASE(release_candidate_schedules_mainnet_only)
{
    struct Restore {
        CBaseChainParams::Network network = Params().NetworkID();
        ~Restore() { SelectParams(network); }
    } restore;
    SelectParams(CBaseChainParams::MAIN);
    BOOST_CHECK_EQUAL(HybridConsensusHeight(), 10000);
    BOOST_CHECK_EQUAL(Params().SplitWorkBootstrapBits(), 0x1d216adeu);
    BOOST_CHECK_EQUAL(Params().SplitStakeLimit().GetCompact(), 0x1c010000u);
    CBlockIndex parent, work, stake;
    parent.nHeight = 9998;
    BOOST_CHECK(!IsSplitDifficultyActive(&parent));
    ++parent.nHeight;
    BOOST_REQUIRE(IsSplitDifficultyActive(&parent));
    work.nBits = GetNextWorkRequired(&parent, BlockProof::Work);
    stake.nBits = GetNextWorkRequired(&parent, BlockProof::Stake);
    BOOST_CHECK_EQUAL(work.nBits, 0x1d216adeu);
    BOOST_CHECK_EQUAL(stake.nBits, 0x1c010000u);
    BOOST_CHECK(GetHybridBlockScore(work, BlockProof::Work) == GetHybridBlockScore(stake, BlockProof::Stake));
    for (auto network : {CBaseChainParams::TESTNET, CBaseChainParams::REGTEST, CBaseChainParams::UNITTEST}) {
        SelectParams(network);
        BOOST_CHECK_EQUAL(HybridConsensusHeight(), 0);
        BOOST_CHECK_EQUAL(Params().SplitDifficultyHeight(), 0);
        BOOST_CHECK_EQUAL(Params().SplitWorkBootstrapBits(), 0u);
        BOOST_CHECK(Params().SplitStakeLimit() == 0);
    }
}

BOOST_AUTO_TEST_CASE(split_difficulty_stake_weight_is_monotonic_without_rewriting_legacy)
{
    uint256 target;
    target.SetCompact(0x207fffff);
    BOOST_CHECK(!stakeTargetHit(target, 100, target, true));
    BOOST_CHECK(stakeTargetHit(target - 1, 100, target, true));
    BOOST_CHECK(stakeTargetHit(target, 200, target, true));
    BOOST_CHECK(!stakeTargetHit(target, 300, target, false));
    BOOST_CHECK(stakeTargetHit(target, 300, target, true));
    BOOST_CHECK(stakeTargetHit(~uint256(0), 300, target, true));
    BOOST_CHECK(!stakeTargetHit(uint256(0), 99, target, true));
}

BOOST_AUTO_TEST_CASE(hybrid_fork_score_reconciles_reference_units_exactly)
{
    CBlockIndex work, stake;
    work.nBits = 0x1d173c32;
    stake.nBits = 0x1c04bc95;
    BOOST_CHECK(GetBlockProof(stake) > GetBlockProof(work) * 24);
    const auto workScore = GetHybridBlockScore(work, BlockProof::Work);
    const auto stakeScore = GetHybridBlockScore(stake, BlockProof::Stake);
    BOOST_CHECK(workScore == stakeScore);
    BOOST_CHECK(workScore * uint32_t(24) > stakeScore);
    BOOST_CHECK(workScore + stakeScore > workScore);

    CBlockIndex easy, hard;
    easy.nBits = 0x2100ffff; // uint256-domain boundary, not a network target
    hard.nBits = 0x01010000; // target = 1
    BOOST_CHECK(GetHybridBlockScore(easy, BlockProof::Stake) == uint512(184847936));
    BOOST_CHECK(GetHybridBlockScore(hard, BlockProof::Work) > uint512(~uint256(0)));
    BOOST_CHECK_EQUAL(RegtestHybridScoreHeight(), 0);
}

BOOST_AUTO_TEST_CASE(hybrid_stake_modifier_is_relative_to_the_candidate_branch)
{
    CBlockIndex origin, left, leftLater, right, rightLater, missing;
    origin.nHeight = 10;
    origin.nTime = 1000;
    const auto set = [](CBlockIndex& block, CBlockIndex& parent, unsigned int time, uint64_t value, bool generated) {
        block.pprev = &parent;
        block.nHeight = parent.nHeight + 1;
        block.nTime = time;
        block.SetStakeModifier(value, generated);
    };
    set(left, origin, 1050, 10, true); // not mature yet
    set(leftLater, left, 1100, 11, true);
    set(right, origin, 1110, 21, true);
    set(rightLater, right, 1120, 22, true);
    set(missing, origin, 1200, 0, false);

    BOOST_CHECK(FindBranchStakeModifier(&origin, &leftLater, 100) == &leftLater);
    BOOST_CHECK(FindBranchStakeModifier(&origin, &rightLater, 100) == &right);
    BOOST_CHECK(FindBranchStakeModifier(&left, &rightLater, 10) == nullptr);
    BOOST_CHECK(FindBranchStakeModifier(&origin, &missing, 100) == nullptr);
    BOOST_CHECK(FindBranchStakeModifier(&origin, &left, 100) == nullptr);
}

BOOST_AUTO_TEST_CASE(hybrid_stake_origin_comes_from_candidate_utxo_view)
{
    const uint256 leftHash(11), rightHash(12);
    const COutPoint input(uint256(20), 0);
    CBlockIndex common, leftOrigin, rightOrigin, leftParent, rightParent;
    leftOrigin.nHeight = rightOrigin.nHeight = 1;
    leftOrigin.pprev = rightOrigin.pprev = &common;
    leftParent.nHeight = rightParent.nHeight = 2;
    leftParent.pprev = &leftOrigin;
    rightParent.pprev = &rightOrigin;
    leftParent.phashBlock = &leftHash;
    rightParent.phashBlock = &rightHash;

    CCoinsView base;
    CCoinsViewCache left(&base), right(&base);
    left.SetBestBlock(leftHash);
    right.SetBestBlock(rightHash);
    {
        auto coins = right.ModifyCoins(input.hash);
        coins->nHeight = 1;
        coins->vout.emplace_back(COIN, CScript() << OP_TRUE);
    }

    CTxOut output;
    BOOST_CHECK(ResolveBranchStakeCoin(input, &rightParent, right, output) == &rightOrigin);
    BOOST_CHECK_EQUAL(output.nValue, COIN);
    BOOST_CHECK(ResolveBranchStakeCoin(input, &leftParent, left, output) == nullptr);
    BOOST_CHECK(ResolveBranchStakeCoin(input, &leftParent, right, output) == nullptr);
    BOOST_CHECK(ResolveBranchStakeCoin(COutPoint(input.hash, 1), &rightParent, right, output) == nullptr);
}

BOOST_AUTO_TEST_CASE(public_stake_rehearsal_checks_a_real_kernel_at_the_candidate_bound)
{
    struct Restore {
        CBaseChainParams::Network network = Params().NetworkID();
        ~Restore() { SelectParams(network); }
    } restore;
    SelectParams(CBaseChainParams::MAIN);
    BOOST_REQUIRE(Params().NetworkID() == CBaseChainParams::MAIN);
    const uint256 parentHash(123), otherHash(124);
    const COutPoint input(uint256(456), 0);
    CBlockIndex origin, parent, other;
    origin.nHeight = 8880;
    origin.nTime = 100000;
    parent.nHeight = other.nHeight = origin.nHeight + 1;
    parent.pprev = other.pprev = &origin;
    parent.nTime = other.nTime = origin.nTime + 7200;
    parent.phashBlock = &parentHash;
    other.phashBlock = &otherHash;
    parent.SetStakeModifier(17, true);
    other.SetStakeModifier(19, true);

    CCoinsView base;
    CCoinsViewCache view(&base), otherView(&base);
    view.SetBestBlock(parentHash);
    otherView.SetBestBlock(otherHash);
    for (auto* candidateView : {&view, &otherView}) {
        auto coins = candidateView->ModifyCoins(input.hash);
        coins->nHeight = origin.nHeight;
        coins->vout.emplace_back(10000 * COIN, CScript() << OP_TRUE);
    }

    CMutableTransaction tx;
    tx.vin.emplace_back(input);
    tx.vout.emplace_back(0, CScript());
    tx.vout.emplace_back(10000 * COIN, CScript() << OP_TRUE);
    CBlock block;
    block.vtx.emplace_back();
    block.vtx.emplace_back(tx);
    block.nBits = 0x1c010000;
    BOOST_REQUIRE(block.IsProofOfStake());

    uint256 proof, alternateProof;
    bool found = false;
    for (unsigned int offset = 1; offset <= 4096; ++offset) {
        block.nTime = parent.nTime + offset;
        if (CheckBranchProofOfStake(block, &parent, view, proof)) {
            found = true;
            break;
        }
    }
    BOOST_REQUIRE(found);
    BOOST_CHECK(!CheckBranchProofOfStake(block, &other, view, alternateProof));
    // A different branch modifier must produce a different kernel hash even
    // if the alternate attempt also happens to meet its target.
    CheckBranchProofOfStake(block, &other, otherView, alternateProof);
    BOOST_CHECK(alternateProof != 0 && alternateProof != proof);
}

BOOST_AUTO_TEST_SUITE_END()
