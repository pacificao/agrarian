// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The PIVX developers
// Copyright (c) 2022-2036 Agrarian Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <algorithm>
#include <math.h>

static uint256 ProofFromBits(unsigned int bits);

int RegtestHybridScoreHeight()
{
    if (Params().NetworkID() != CBaseChainParams::REGTEST)
        return 0;
    const int64_t height = GetArg("-regtestsplitdifficultyheight", 0);
    return height >= Params().FIRST_POS_BLOCK() && height <= Params().LAST_POW_BLOCK() ? height : 0;
}

int HybridConsensusHeight()
{
    const auto& params = Params();
    if (params.NetworkID() == CBaseChainParams::REGTEST)
        return RegtestHybridScoreHeight();
    return PublicHybridConsensusHeight(params);
}

int PublicHybridConsensusHeight(const CChainParams& params)
{
    if (params.NetworkID() == CBaseChainParams::REGTEST)
        return 0;
    const int height = params.SplitDifficultyHeight();
    if (height < params.FIRST_POS_BLOCK() || height > params.LAST_POW_BLOCK())
        return 0;
    const uint256& stakeLimit = params.SplitStakeLimit();
    const unsigned int workBits = params.SplitWorkBootstrapBits();
    uint256 workBootstrap;
    bool negative, overflow;
    workBootstrap.SetCompact(workBits, &negative, &overflow);
    // Limit initial evidence to 64 bits so cumulative scoring is bounded.
    const uint256 minBootstrap = uint256(1) << 192;
    if (stakeLimit < minBootstrap || stakeLimit >= params.ProofOfWorkLimit() ||
        uint256().SetCompact(stakeLimit.GetCompact()) != stakeLimit ||
        negative || overflow || workBootstrap < minBootstrap || workBootstrap > params.ProofOfWorkLimit() ||
        workBootstrap.GetCompact() != workBits)
        return 0;
    return height;
}

uint512 GetHybridBlockScore(const CBlockIndex& block, BlockProof proof)
{
    // Staging fixtures, not mainnet calibration. These are GetBlockProof at
    // compact targets 0x1d173c32 (work) and 0x1c04bc95 (stake).
    const bool publicProfile = Params().NetworkID() != CBaseChainParams::REGTEST && HybridConsensusHeight() != 0;
    const uint64_t workReference = publicProfile ? ProofFromBits(Params().SplitWorkBootstrapBits()).Get64() : 184847936;
    const uint64_t stakeReference = publicProfile ? ProofFromBits(Params().SplitStakeLimit().GetCompact()).Get64() : 232128606112;
    uint512 score(GetBlockProof(block));
    score *= uint512(proof == BlockProof::Stake ? workReference : stakeReference);
    // Even INT_MAX blocks of maximum uint256 work and uint64 references
    // fit in 351 bits. No rounding or wrap affects ordering.
    return score;
}

bool IsSplitDifficultyActive(const CBlockIndex* previous)
{
    if (!previous)
        return false;

    const int64_t activation = HybridConsensusHeight();

    const int64_t height = int64_t(previous->nHeight) + 1;
    return activation >= Params().FIRST_POS_BLOCK() && height >= activation &&
        height <= Params().LAST_POW_BLOCK();
}

unsigned int GetNextSplitWorkRequired(const CBlockIndex* previous, BlockProof proof)
{
    const bool regtest = Params().NetworkID() == CBaseChainParams::REGTEST;
    const uint256& limit = proof == BlockProof::Stake && !regtest
        ? Params().SplitStakeLimit() : Params().ProofOfWorkLimit();
    if (!regtest && HybridConsensusHeight() > 0) {
        const auto bits = proof == BlockProof::Work ? Params().SplitWorkBootstrapBits() : limit.GetCompact();
        return GetNextSplitWorkRequired(previous, proof, limit, HybridConsensusHeight(), bits);
    }
    return GetNextSplitWorkRequired(previous, proof, limit);
}

unsigned int GetNextSplitWorkRequired(const CBlockIndex* previous, BlockProof proof, const uint256& limit)
{
    return GetNextSplitWorkRequired(previous, proof, limit, 1, limit.GetCompact());
}

unsigned int GetNextSplitWorkRequired(const CBlockIndex* previous, BlockProof proof, const uint256& limit,
                                     int firstSampleHeight, unsigned int bootstrapBits)
{
    assert(limit > 0);
    constexpr unsigned int window = 24;
    const bool stake = proof == BlockProof::Stake;
    uint512 sum = 0;
    uint512 lastTarget = 0;
    unsigned int count = 0;
    int64_t newest = 0;
    int64_t oldest = 0;

    // Proof type comes from validated full blocks, never from the header alone.
    for (const CBlockIndex* index = previous; index && index->nHeight >= firstSampleHeight && count < window; index = index->pprev) {
        if (index->IsProofOfStake() != stake)
            continue;
        bool negative, overflow;
        uint256 target;
        target.SetCompact(index->nBits, &negative, &overflow);
        if (negative || overflow || target == 0)
            continue;
        // Preserve the pre-activation sample's timing, but do not carry an
        // easier legacy target across the new proof-specific upper bound.
        const uint512 wideTarget(std::min(target, limit));
        if (count == 0) {
            lastTarget = wideTarget;
            newest = index->GetBlockTime();
        }
        oldest = index->GetBlockTime();
        sum += wideTarget;
        ++count;
    }

    // A dormant/missing stream does not inherit the other proof's difficulty.
    if (count == 0)
        return bootstrapBits;
    if (count == 1)
        return lastTarget.trim256().GetCompact();

    // Two equally spaced streams aim for the existing combined spacing when
    // both are active. Keep the legacy PoS-only regime after the hybrid window.
    const int64_t nominal = (count - 1) * 2 * Params().TargetSpacing();
    const int64_t elapsed = std::max(nominal / 3, std::min(nominal * 3, newest - oldest));
    // Widen before summing/scaling, including on regtest's near-256-bit limit.
    uint512 next = sum / uint512(count);
    next *= static_cast<uint32_t>(elapsed);
    next /= uint512(nominal);
    const uint512 lower = std::max(uint512(1), uint512(lastTarget / uint512(3)));
    const uint512 upper = std::min(uint512(limit), uint512(lastTarget * uint32_t(3)));
    return std::max(lower, std::min(upper, next)).trim256().GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, BlockProof proof)
{
    if (IsSplitDifficultyActive(pindexLast))
        return GetNextSplitWorkRequired(pindexLast, proof);

    if (Params().NetworkID() == CBaseChainParams::REGTEST)
        return pindexLast ? pindexLast->nBits : Params().ProofOfWorkLimit().GetCompact();

    /* current difficulty formula, agrarian - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    uint256 PastDifficultyAverage;
    uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == nullptr || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

    if (pindexLast->nHeight > Params().LAST_POW_BLOCK()) {
        uint256 bnTargetLimit = (~uint256(0) >> 24);
        int64_t nTargetSpacing = 60;
        int64_t nTargetTimespan = 60 * 40;

        int64_t nActualSpacing = 0;
        if (pindexLast->nHeight != 0)
            nActualSpacing = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

        if (nActualSpacing < 0)
            nActualSpacing = 1;

        // ppcoin: target change every block
        // ppcoin: retarget with exponential moving toward target spacing
        uint256 bnNew;
        bnNew.SetCompact(pindexLast->nBits);

        int64_t nInterval = nTargetTimespan / nTargetSpacing;
        bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
        bnNew /= ((nInterval + 1) * nTargetSpacing);

        if (bnNew <= 0 || bnNew > bnTargetLimit)
            bnNew = bnTargetLimit;

        return bnNew.GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev* CountBlocks) + (uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == nullptr) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks* Params().TargetSpacing();

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit()) {
        bnNew = Params().ProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
        return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget) {
        if (Params().MineBlocksOnDemand())
            return false;
        else
            return error("CheckProofOfWork() : hash doesn't match nBits");
    }

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    return ProofFromBits(block.nBits);
}

static uint256 ProofFromBits(unsigned int bits)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(bits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
