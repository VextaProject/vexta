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

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // Vexta is SHA256D-only. Use one global averaging window.
    if (pindexLast == nullptr) {
        return InitialDifficulty(params);
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
