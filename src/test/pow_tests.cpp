// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2020 The DigiByte Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// Copyright (c) 2015-2020 The DigiByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);

    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nVersion = 1;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);

        // Create random block hash
        const uint256 randomhash = GetRandHash();
        uint256* ptr = new uint256();
        *ptr = randomhash;

        blocks[i].phashBlock = ptr;
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }

    for (int i = 0; i < 10000; ++i) {
        delete blocks[i].phashBlock;
    }
}

void sanity_check_chainparams(const ArgsManager& args, std::string chainName)
{
    const auto chainParams = CreateChainParams(args, chainName);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::SIGNET);
}


BOOST_AUTO_TEST_CASE(ASERT_behavior_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto consensus = chainParams->GetConsensus();

    std::vector<CBlockIndex> blocks(6501);

    arith_uint256 anchorTargetValue = UintToArith256(consensus.powLimit);
    anchorTargetValue >>= 8;
    const unsigned int anchorBits = anchorTargetValue.GetCompact();
    const int64_t startTime = 1800000000;

    for (int i = 0; i <= 6500; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = anchorBits;
    }

    // Block 6499 is still governed by the legacy DAA.
    const unsigned int preActivationBits =
        GetNextWorkRequired(&blocks[6498], nullptr, consensus);
    BOOST_CHECK_EQUAL(preActivationBits, anchorBits);

    // Block 6500 is the first ASERT block. Ideal 10-minute spacing
    // must preserve the anchor target.
    const unsigned int idealBits =
        GetNextWorkRequired(&blocks[6499], nullptr, consensus);
    BOOST_CHECK_EQUAL(idealBits, anchorBits);

    // Block 6501 after a very fast previous block: target must decrease
    // (difficulty increases).
    blocks[6500].nTime =
        blocks[6499].nTime + consensus.nPowTargetSpacing / 10;

    const unsigned int fastBits =
        GetNextWorkRequired(&blocks[6500], nullptr, consensus);

    arith_uint256 fastTarget;
    fastTarget.SetCompact(fastBits);

    arith_uint256 anchorTarget;
    anchorTarget.SetCompact(anchorBits);

    BOOST_CHECK(fastTarget < anchorTarget);

    // Block 6501 after a slow previous block: target must increase
    // (difficulty decreases).
    blocks[6500].nTime =
        blocks[6499].nTime + consensus.nPowTargetSpacing * 3;

    const unsigned int slowBits =
        GetNextWorkRequired(&blocks[6500], nullptr, consensus);

    arith_uint256 slowTarget;
    slowTarget.SetCompact(slowBits);

    BOOST_CHECK(slowTarget > anchorTarget);

    // Extreme delay must never produce a target above powLimit.
    blocks[6500].nTime =
        blocks[6499].nTime + consensus.asertHalfLife * 16;

    const unsigned int extremeSlowBits =
        GetNextWorkRequired(&blocks[6500], nullptr, consensus);

    BOOST_CHECK_EQUAL(
        extremeSlowBits,
        UintToArith256(consensus.powLimit).GetCompact());
}


BOOST_AUTO_TEST_CASE(ASERT_reference_vector_run04_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 2;
    consensus.asertHalfLife = 2 * 24 * 60 * 60;

    std::vector<CBlockIndex> blocks(3);

    blocks[0].pprev = nullptr;
    blocks[0].nHeight = 0;
    blocks[0].nTime = 0;
    blocks[0].nBits = 0x01010000;

    blocks[1].pprev = &blocks[0];
    blocks[1].nHeight = 1;
    blocks[1].nTime = 600;
    blocks[1].nBits = 0x01010000;

    // BCHN official ASERT run04 vector:
    // anchor height = 1
    // anchor ancestor time = 0
    // evaluation height = 2
    // evaluation time = 174000
    // expected next target = 0x01020000
    blocks[2].pprev = &blocks[1];
    blocks[2].nHeight = 2;
    blocks[2].nTime = 174000;
    blocks[2].nBits = 0x01010000;

    const unsigned int result =
        GetNextWorkRequired(&blocks[2], nullptr, consensus);

    BOOST_CHECK_EQUAL(result, 0x01020000U);
}


BOOST_AUTO_TEST_CASE(ASERT_reference_vector_run05_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 2;
    consensus.asertHalfLife = 2 * 24 * 60 * 60;

    std::vector<CBlockIndex> blocks(291);

    blocks[0].pprev = nullptr;
    blocks[0].nHeight = 0;
    blocks[0].nTime = 0;
    blocks[0].nBits = 0x1d00ffff;

    blocks[1].pprev = &blocks[0];
    blocks[1].nHeight = 1;
    blocks[1].nTime = 600;
    blocks[1].nBits = 0x1d00ffff;

    // BCHN official ASERT run05 vector, iteration 1:
    blocks[2].pprev = &blocks[1];
    blocks[2].nHeight = 2;
    blocks[2].nTime = 0;
    blocks[2].nBits = 0x1d00ffff;

    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[2], nullptr, consensus),
        0x1d00fec5U);

    // BCHN official ASERT run05 vector, iteration 2:
    // evaluation height = 290, time = 0
    for (int h = 3; h <= 290; ++h) {
        blocks[h].pprev = &blocks[h - 1];
        blocks[h].nHeight = h;
        blocks[h].nTime = 0;
        blocks[h].nBits = 0x1d00ffff;
    }

    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[290], nullptr, consensus),
        0x1c7f62c0U);
}


BOOST_AUTO_TEST_CASE(ASERT_reference_vector_run06_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 2;
    consensus.asertHalfLife = 2 * 24 * 60 * 60;

    std::vector<CBlockIndex> blocks(13);

    blocks[0].pprev = nullptr;
    blocks[0].nHeight = 0;
    blocks[0].nTime = 0;
    blocks[0].nBits = 0x1802aee8;

    blocks[1].pprev = &blocks[0];
    blocks[1].nHeight = 1;
    blocks[1].nTime = 600;
    blocks[1].nBits = 0x1802aee8;

    const int64_t times[] = {
        0, 600,
        1200, 1310, 1327, 1739, 2219, 2604,
        2746, 7099, 7099, 7657, 8626
    };

    const unsigned int expected[] = {
        0,
        0,
        0x1802aee8U,
        0x1802ad91U,
        0x1802abf8U,
        0x1802ab73U,
        0x1802ab20U,
        0x1802aa89U,
        0x1802a94bU,
        0x1802b39cU,
        0x1802b1f2U,
        0x1802b1d4U,
        0x1802b2dbU
    };

    for (int h = 2; h <= 12; ++h) {
        blocks[h].pprev = &blocks[h - 1];
        blocks[h].nHeight = h;
        blocks[h].nTime = times[h];
        blocks[h].nBits = 0x1802aee8;

        BOOST_CHECK_EQUAL(
            GetNextWorkRequired(&blocks[h], nullptr, consensus),
            expected[h]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
