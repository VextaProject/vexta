// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2020 The DigiByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>
#include <crypto/randomx_hash.h>
#include <crypto/common.h>
#include <consensus/consensus.h>
#include <chainparams.h>
#include <hash.h>
#include <streams.h>
#include <tinyformat.h>
#include <version.h>
#include <arith_uint256.h>

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

int CBlockHeader::GetAlgo() const
{
    switch (nVersion & BLOCK_VERSION_ALGO) {
        case BLOCK_VERSION_SHA256D:
            return ALGO_SHA256D;
        case BLOCK_VERSION_RANDOMX:
            return ALGO_RANDOMX;
        default:
            return ALGO_UNKNOWN;
    }
}

bool CBlockHeader::GetRandomXPoWHash(const uint256& seed, uint256& result) const
{
    VextaRandomXHasher hasher(seed.data(), seed.size());

    if (!hasher.IsValid()) {
        return false;
    }

    return GetRandomXPoWHash(hasher, result);
}

bool CBlockHeader::GetRandomXPoWHash(
    VextaRandomXHasher& hasher,
    uint256& result) const
{
    CDataStream stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << *this;

    return hasher.Hash(
        stream.data(),
        stream.size(),
        result.begin());
}

std::string CBlock::ToString(const Consensus::Params& params) const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}


