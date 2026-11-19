// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"
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

BOOST_AUTO_TEST_SUITE_END()
