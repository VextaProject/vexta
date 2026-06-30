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

unsigned int InitialDifficulty(const Consensus::Params& params, int algo)
{
    const auto& it = params.initialTarget.find(algo);
    if (it == params.initialTarget.end())
        return PowLimit(params);
    return UintToArith256(it->second).GetCompact();
}

unsigned int GetNextWorkRequiredV4(const CBlockIndex* pindexLast, const Consensus::Params& params, int algo)
{
    // find first block in averaging interval
    // Go back by what we want to be nAveragingInterval blocks per algo
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < NUM_ALGOS*params.nAveragingInterval; i++)
    {
        pindexFirst = pindexFirst->pprev;
    }

    const CBlockIndex* pindexPrevAlgo = GetLastBlockIndexForAlgoFast(pindexLast, params, algo);
    if (pindexPrevAlgo == nullptr || pindexFirst == nullptr)
    {
        return InitialDifficulty(params, algo);
    }

    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = pindexLast-> GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    nActualTimespan = params.nAveragingTargetTimespanV4 + (nActualTimespan - params.nAveragingTargetTimespanV4)/4;

    if (nActualTimespan < params.nMinActualTimespanV4)
        nActualTimespan = params.nMinActualTimespanV4;
    if (nActualTimespan > params.nMaxActualTimespanV4)
        nActualTimespan = params.nMaxActualTimespanV4;

    //Global retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrevAlgo->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.nAveragingTargetTimespanV4;

    //Per-algo retarget
    int nAdjustments = pindexPrevAlgo->nHeight + NUM_ALGOS - 1 - pindexLast->nHeight;
    if (nAdjustments > 0)
    {
        for (int i = 0; i < nAdjustments; i++)
        {
            bnNew *= 100;
            bnNew /= (100 + params.nLocalTargetAdjustment);
        }
    }
    else if (nAdjustments < 0)//make it easier
    {
        for (int i = 0; i < -nAdjustments; i++)
        {
            bnNew *= (100 + params.nLocalTargetAdjustment);
            bnNew /= 100;
            if (bnNew > UintToArith256(params.powLimit)) {
              bnNew = UintToArith256(params.powLimit);
              break;
            }            
        }
    }

    if (bnNew > UintToArith256(params.powLimit))
    {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, int algo)
{
    // Vexta is SHA256D-only and starts from modern difficulty logic.
    return GetNextWorkRequiredV4(pindexLast, params, ALGO_SHA256D);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

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

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, const Consensus::Params& params, int algo)
{
    for (; pindex; pindex = pindex->pprev)
    {
        if (pindex->GetAlgo() != algo)
            continue;
        // ignore special min-difficulty testnet blocks
        if (params.fPowAllowMinDifficultyBlocks &&
            pindex->pprev &&
            pindex->nTime > pindex->pprev->nTime + params.nTargetSpacing*2)
        {
            continue;
        }
        return pindex;
    }
    return nullptr;
}

const CBlockIndex* GetLastBlockIndexForAlgoFast(const CBlockIndex* pindex, const Consensus::Params& params, int algo)
{
    for (; pindex; pindex = pindex->lastAlgoBlocks[algo])
    {
        if (pindex->GetAlgo() != algo)
            continue;
        if (params.fPowAllowMinDifficultyBlocks &&
            pindex->pprev &&
            pindex->nTime > pindex->pprev->nTime + params.nTargetSpacing*2)
        {
            pindex = pindex->pprev;
            continue;
        }
        return pindex;
    }

    return nullptr;
}

uint256 GetPoWAlgoHash(const CBlockHeader& block)
{
    return block.GetPoWAlgoHash(Params().GetConsensus());
}
