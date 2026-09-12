// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2020 The DigiByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>
#include <logging.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <chainparams.h>

inline unsigned int PowLimit(const Consensus::Params& params)
{
    return UintToArith256(params.powLimit).GetCompact();
}

unsigned int InitialDifficulty(const Consensus::Params& params)
{
    return PowLimit(params);
}

unsigned int CalculateASERT(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // Anchor ASERT to the last block before activation.
    const int anchorHeight = params.asertActivationHeight - 1;
    const CBlockIndex* pindexAnchor = pindexLast->GetAncestor(anchorHeight);

    if (pindexAnchor == nullptr) {
        return pindexLast->nBits;
    }

    const int64_t heightDiff = pindexLast->nHeight - pindexAnchor->nHeight;
    const int64_t timeDiff = pindexLast->GetBlockTime() - pindexAnchor->pprev->GetBlockTime();

    // ASERT exponent in fixed-point 16.16:
    // exponent = (timeDiff - idealTime) / halfLife
    const int64_t idealTime = (heightDiff + 1) * params.nPowTargetSpacing;
    const int64_t exponent = ((timeDiff - idealTime) * 65536) / params.asertHalfLife;

    int64_t shifts = exponent >> 16;
    const uint16_t frac = static_cast<uint16_t>(exponent);

    // Approximation of 2^(frac/65536), scaled by 65536.
    const uint64_t factor =
        65536ULL +
        ((195766423245049ULL * frac +
          971821376ULL * frac * frac +
          5127ULL * frac * frac * frac +
          (1ULL << 47)) >> 48);

    arith_uint256 target;
    target.SetCompact(pindexAnchor->nBits);
    target *= factor;

    // factor is scaled by 2^16.
    shifts -= 16;

    const arith_uint256 powLimit = UintToArith256(params.powLimit);

    if (shifts <= 0) {
        if (-shifts >= 256) {
            target = 1;
        } else {
            target >>= -shifts;
        }
    } else {
        if (shifts >= 256 || target > (powLimit >> shifts)) {
            target = powLimit;
        } else {
            target <<= shifts;
        }
    }

    if (target == 0) {
        target = 1;
    }
    if (target > powLimit) {
        target = powLimit;
    }

    return target.GetCompact();
}

static bool ShouldTriggerFastRise(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    // Trigger when at least 3 of the last 4 solve intervals are below
    // 20% of the target spacing (120 seconds on Vexta).
    // Zero or negative raw intervals also count as fast to prevent
    // backward/equal timestamp manipulation from bypassing the trigger.
    const int64_t fastThreshold = params.nPowTargetSpacing / 5;
    int fastIntervals = 0;

    const CBlockIndex* pindex = pindexLast;
    for (int i = 0; i < 4; ++i) {
        if (pindex == nullptr || pindex->pprev == nullptr) {
            return false;
        }

        const int64_t solveTime =
            pindex->GetBlockTime() - pindex->pprev->GetBlockTime();

        if (solveTime < fastThreshold) {
            ++fastIntervals;
        }

        pindex = pindex->pprev;
    }

    return fastIntervals >= 3;
}

static unsigned int ApplyFastRiseProtection(
    const CBlockIndex* pindexLast,
    unsigned int asertBits,
    const Consensus::Params& params)
{
    if (pindexLast->nHeight + 1 < params.fastRiseActivationHeight) {
        return asertBits;
    }

    if (!ShouldTriggerFastRise(pindexLast, params)) {
        return asertBits;
    }

    arith_uint256 asertTarget;
    asertTarget.SetCompact(asertBits);

    arith_uint256 fastTarget;
    fastTarget.SetCompact(pindexLast->nBits);
    fastTarget >>= 1;

    if (fastTarget == 0) {
        fastTarget = 1;
    }

    // Never weaken standard ASERT. Under a Fast-Rise trigger, require
    // at least one 2x difficulty step relative to the previous block.
    if (asertTarget < fastTarget) {
        return asertBits;
    }

    return fastTarget.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, int algo)
{
    (void)algo;
    // Vexta is SHA256D-only.
    if (pindexLast == nullptr) {
        return InitialDifficulty(params);
    }

    // ASERT activates for the block at asertActivationHeight.
    // pindexLast is the previous block, so activation starts when
    // the next block height reaches the configured activation height.
    if (!params.fPowNoRetargeting &&
        !params.fEasyPow &&
        pindexLast->nHeight + 1 >= params.asertActivationHeight) {
        const unsigned int asertBits = CalculateASERT(pindexLast, params);
        return ApplyFastRiseProtection(pindexLast, asertBits, params);
    }

    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < params.difficultyAveragingWindow; i++) {
        pindexFirst = pindexFirst->pprev;
    }

    if (pindexFirst == nullptr) {
        return InitialDifficulty(params);
    }

    if (params.fPowNoRetargeting || params.fEasyPow) {
        return pindexLast->nBits;
    }

    int64_t nActualTimespan = pindexLast->GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    nActualTimespan = params.difficultyTargetTimespan + (nActualTimespan - params.difficultyTargetTimespan) / 4;

    if (nActualTimespan < params.difficultyMinActualTimespan)
        nActualTimespan = params.difficultyMinActualTimespan;
    if (nActualTimespan > params.difficultyMaxActualTimespan)
        nActualTimespan = params.difficultyMaxActualTimespan;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.difficultyTargetTimespan;

    if (bnNew > UintToArith256(params.powLimit)) {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}

const CBlockIndex* GetLastBlockIndexForAlgoFast(const CBlockIndex* pindex, int algo)
{
    if (algo < 0 || algo >= NUM_ALGOS_IMPL) {
        return nullptr;
    }

    while (pindex) {
        if (pindex->GetAlgo() == algo) {
            return pindex;
        }

        pindex = pindex->lastAlgoBlocks[algo];
    }

    return nullptr;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
