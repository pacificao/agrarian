// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
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
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history[24], BlockProof::Stake),
                      Params().ProofOfWorkLimit().GetCompact());

    // The observed mainnet incident comprised 99 stake blocks over 155 s.
    for (size_t i = 25; i < history.size(); ++i) {
        history[i].SetProofOfStake();
        history[i].nTime = history[24].nTime + (i - 25) * 155 / 98;
        history[i].nBits = 0x1c04fd77;
    }
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history.back(), BlockProof::Work), work);
    BOOST_CHECK_EQUAL(GetNextSplitWorkRequired(&history[25], BlockProof::Stake), 0x1c04fd77u);
    uint256 prior, next;
    prior.SetCompact(history.back().nBits);
    next.SetCompact(GetNextSplitWorkRequired(&history.back(), BlockProof::Stake));
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

BOOST_AUTO_TEST_SUITE_END()
