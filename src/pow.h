// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2020 The DigiByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DIGIBYTE_POW_H
#define DIGIBYTE_POW_H

#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

unsigned int InitialDifficulty(const Consensus::Params& params);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&, int algo);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);
const CBlockIndex* GetLastBlockIndexForAlgoFast(const CBlockIndex* pindex, int algo);

/** Return the block height whose hash is used as the RandomX seed key. */
int GetRandomXSeedHeight(int blockHeight, const Consensus::Params& params);

/**
 * Resolve the RandomX seed key from the previous block index.
 *
 * The seed is always derived from an already-known ancestor block hash.
 * Returns false if the requested seed ancestor cannot be resolved.
 */
bool GetRandomXSeed(
    const CBlockIndex* pindexPrev,
    int blockHeight,
    const Consensus::Params& params,
    uint256& seed);

#endif // DIGIBYTE_POW_H
