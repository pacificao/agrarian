// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2022-2036 Agrarian Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef AGRARIAN_BITCOIN_POW_H
#define AGRARIAN_BITCOIN_POW_H
#include <cstdint>
#include <cstddef>
class CBlockHeader;
class CBlockIndex;
class uint256;
class uint512;
class arith_uint256;

// Define difficulty retarget algorithms
enum DiffMode {
    DIFF_DEFAULT = 0, // Default to invalid 0
    DIFF_BTC = 1,     // Retarget every x blocks (Bitcoin style)
    DIFF_KGW = 2,     // Retarget using Kimoto Gravity Well
    DIFF_DGW = 3,     // Retarget using Dark Gravity Wave v3
};

enum class BlockProof { Work, Stake };

/** Whether the next block uses independent hybrid difficulty rules. */
bool IsSplitDifficultyActive(const CBlockIndex* previous);
int RegtestHybridScoreHeight();
uint512 GetHybridBlockScore(const CBlockIndex& block, BlockProof proof);

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, BlockProof proof);

/** Candidate per-proof retarget, exposed for deterministic consensus tests. */
unsigned int GetNextSplitWorkRequired(const CBlockIndex* previous, BlockProof proof);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
uint256 GetBlockProof(const CBlockIndex& block);

#endif // AGRARIAN_BITCOIN_POW_H
