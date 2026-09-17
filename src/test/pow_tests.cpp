// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2020 The DigiByte Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// Copyright (c) 2015-2020 The DigiByte Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <primitives/block.h>
#include <util/strencodings.h>
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


BOOST_AUTO_TEST_CASE(MultiAlgo_chainwork_normalization_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.randomXActivationHeight = 40;
    consensus.multiAlgoChainworkScaleNumerator = 1;
    consensus.multiAlgoChainworkScaleDenominator = 1;

    std::vector<CBlockIndex> blocks(40);

    arith_uint256 shaTarget = UintToArith256(consensus.powLimit);
    shaTarget >>= 8;
    const unsigned int shaBits = shaTarget.GetCompact();

    const int64_t startTime = 1800000000;

    CBlockIndex* latest[NUM_ALGOS_IMPL]{};

    for (int i = 0; i < 40; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime =
            startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = shaBits;
        blocks[i].nVersion =
            BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

        for (int algo = 0; algo < NUM_ALGOS_IMPL; ++algo) {
            blocks[i].lastAlgoBlocks[algo] = latest[algo];
        }

        latest[ALGO_SHA256D] = &blocks[i];
    }

    // Before multi-algo activation, consensus-aware proof must remain
    // bit-for-bit identical to the historical SHA256D raw proof.
    BOOST_CHECK(
        GetBlockProof(blocks[39], consensus) ==
        GetBlockProof(blocks[39]));

    // Height 40 is the first multi-algo height. Build two alternative
    // children of the same parent: one SHA256D and one RandomX.
    CBlockIndex shaChild;
    shaChild.pprev = &blocks[39];
    shaChild.nHeight = 40;
    shaChild.nTime =
        blocks[39].nTime + consensus.nPowTargetSpacing;
    shaChild.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;
    shaChild.nBits =
        GetNextWorkRequired(
            &blocks[39], nullptr, consensus, ALGO_SHA256D);

    CBlockIndex randomXChild;
    randomXChild.pprev = &blocks[39];
    randomXChild.nHeight = 40;
    randomXChild.nTime =
        blocks[39].nTime + consensus.nPowTargetSpacing;
    randomXChild.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_RANDOMX;
    randomXChild.nBits =
        GetNextWorkRequired(
            &blocks[39], nullptr, consensus, ALGO_RANDOMX);

    // The two algorithms deliberately have different raw targets here.
    BOOST_REQUIRE_NE(shaChild.nBits, randomXChild.nBits);

    // Raw proof is therefore different.
    BOOST_REQUIRE(
        GetBlockProof(shaChild) !=
        GetBlockProof(randomXChild));

    // But normalized chainwork for competing children of the same parent
    // must be identical. Otherwise one PoW family could win chain selection
    // merely because its numeric difficulty scale differs.
    BOOST_CHECK(
        GetBlockProof(shaChild, consensus) ==
        GetBlockProof(randomXChild, consensus));

    // A fixed branch-independent rational scale can preserve chainwork
    // continuity across the activation boundary. Derive an exact scale for
    // this synthetic test state only; production parameters are configured
    // separately in chainparams.
    const arith_uint256 preActivationWork =
        GetBlockProof(blocks[39], consensus);
    const arith_uint256 unscaledPostActivationWork =
        GetBlockProof(shaChild, consensus);

    BOOST_REQUIRE(preActivationWork > 0);
    BOOST_REQUIRE(unscaledPostActivationWork > 0);
    BOOST_REQUIRE_LE(preActivationWork.bits(), 32U);
    BOOST_REQUIRE_LE(unscaledPostActivationWork.bits(), 32U);

    consensus.multiAlgoChainworkScaleNumerator =
        static_cast<uint32_t>(preActivationWork.GetLow64());
    consensus.multiAlgoChainworkScaleDenominator =
        static_cast<uint32_t>(unscaledPostActivationWork.GetLow64());

    BOOST_CHECK(
        GetBlockProof(shaChild, consensus) ==
        preActivationWork);
    BOOST_CHECK(
        GetBlockProof(randomXChild, consensus) ==
        preActivationWork);

}



BOOST_AUTO_TEST_CASE(MultiAlgo_chainwork_scale_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.randomXActivationHeight = 2;

    CBlockIndex genesisLike;
    genesisLike.nHeight = 0;
    genesisLike.nTime = 1800000000;
    genesisLike.nBits =
        UintToArith256(consensus.powLimit).GetCompact();
    genesisLike.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

    CBlockIndex parent;
    parent.pprev = &genesisLike;
    parent.nHeight = 1;
    parent.nTime =
        genesisLike.nTime + consensus.nPowTargetSpacing;
    parent.nBits =
        UintToArith256(consensus.powLimit).GetCompact();
    parent.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;
    parent.lastAlgoBlocks[ALGO_SHA256D] = &genesisLike;

    CBlockIndex child;
    child.pprev = &parent;
    child.nHeight = 2;
    child.nTime =
        parent.nTime + consensus.nPowTargetSpacing;
    child.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;
    child.nBits =
        GetNextWorkRequired(
            &parent,
            nullptr,
            consensus,
            ALGO_SHA256D);

    consensus.multiAlgoChainworkScaleNumerator = 1;
    consensus.multiAlgoChainworkScaleDenominator = 1;
    const arith_uint256 unscaled =
        GetBlockProof(child, consensus);

    BOOST_REQUIRE(unscaled > 0);

    consensus.multiAlgoChainworkScaleNumerator = 2;
    consensus.multiAlgoChainworkScaleDenominator = 1;
    BOOST_CHECK(
        GetBlockProof(child, consensus) ==
        unscaled * 2);

    consensus.multiAlgoChainworkScaleNumerator = 1;
    consensus.multiAlgoChainworkScaleDenominator = 2;
    BOOST_CHECK(
        GetBlockProof(child, consensus) ==
        unscaled / 2);

    consensus.multiAlgoChainworkScaleNumerator = 1;
    consensus.multiAlgoChainworkScaleDenominator = 0;
    BOOST_CHECK(
        GetBlockProof(child, consensus) == 0);

    consensus.multiAlgoChainworkScaleNumerator = 0;
    consensus.multiAlgoChainworkScaleDenominator = 1;
    BOOST_CHECK(
        GetBlockProof(child, consensus) == 0);

    // Force the overflow guard using consensus state that produces an
    // extremely large normalized work value. Post-activation chainwork is
    // derived from expected per-algo targets, not from child.nBits.
    auto overflowConsensus = consensus;
    overflowConsensus.fPowNoRetargeting = true;
    overflowConsensus.randomXActivationHeight = 2;

    arith_uint256 tinyTarget(1);
    const unsigned int tinyBits = tinyTarget.GetCompact();
    overflowConsensus.randomXInitialTarget =
        ArithToUint256(tinyTarget);

    CBlockIndex overflowParent = parent;
    overflowParent.nBits = tinyBits;
    overflowParent.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

    CBlockIndex overflowChild = child;
    overflowChild.pprev = &overflowParent;
    overflowChild.nHeight = 2;

    overflowConsensus.multiAlgoChainworkScaleNumerator =
        std::numeric_limits<uint32_t>::max();
    overflowConsensus.multiAlgoChainworkScaleDenominator = 1;

    BOOST_CHECK(
        GetBlockProof(overflowChild, overflowConsensus) == 0);
}



BOOST_AUTO_TEST_CASE(MultiAlgo_chainwork_branch_accumulation_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    // Test-only activation. Mainnet RandomX remains disabled.
    consensus.randomXActivationHeight = 40;

    std::vector<CBlockIndex> history(40);

    arith_uint256 shaTarget = UintToArith256(consensus.powLimit);
    shaTarget >>= 8;
    const unsigned int shaBits = shaTarget.GetCompact();

    const int64_t startTime = 1800000000;

    CBlockIndex* latest[NUM_ALGOS_IMPL]{};

    for (int i = 0; i < 40; ++i) {
        CBlockIndex& block = history[i];

        block.pprev = i ? &history[i - 1] : nullptr;
        block.nHeight = i;
        block.nTime = startTime + i * consensus.nPowTargetSpacing;
        block.nBits = shaBits;
        block.nVersion =
            BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

        for (int algo = 0; algo < NUM_ALGOS_IMPL; ++algo) {
            block.lastAlgoBlocks[algo] = latest[algo];
        }

        latest[ALGO_SHA256D] = &block;

        block.nChainWork =
            (block.pprev ? block.pprev->nChainWork : arith_uint256{0}) +
            GetBlockProof(block, consensus);
    }

    auto buildChild =
        [&](CBlockIndex& child,
            CBlockIndex& parent,
            int algo,
            uint32_t time) {
            child.pprev = &parent;
            child.nHeight = parent.nHeight + 1;
            child.nTime = time;
            child.nVersion =
                BLOCK_VERSION_DEFAULT |
                (algo == ALGO_RANDOMX
                    ? BLOCK_VERSION_RANDOMX
                    : BLOCK_VERSION_SHA256D);

            for (int i = 0; i < NUM_ALGOS_IMPL; ++i) {
                child.lastAlgoBlocks[i] = parent.lastAlgoBlocks[i];
            }

            child.nBits =
                GetNextWorkRequired(
                    &parent,
                    nullptr,
                    consensus,
                    algo);

            child.lastAlgoBlocks[algo] = &child;

            child.nChainWork =
                parent.nChainWork +
                GetBlockProof(child, consensus);
        };

    CBlockIndex shaBranch1;
    CBlockIndex randomXBranch1;

    const uint32_t firstTime =
        history[39].nTime + consensus.nPowTargetSpacing;

    buildChild(
        shaBranch1,
        history[39],
        ALGO_SHA256D,
        firstTime);

    buildChild(
        randomXBranch1,
        history[39],
        ALGO_RANDOMX,
        firstTime);

    // Native SHA256D and RandomX targets differ, but siblings over the same
    // parent must receive exactly the same normalized chainwork increment.
    BOOST_REQUIRE(
        GetBlockProof(shaBranch1) !=
        GetBlockProof(randomXBranch1));

    BOOST_CHECK(
        GetBlockProof(shaBranch1, consensus) ==
        GetBlockProof(randomXBranch1, consensus));

    BOOST_CHECK(
        shaBranch1.nChainWork ==
        randomXBranch1.nChainWork);

    // Extend both competing branches with the opposite algorithm.
    CBlockIndex shaThenRandomX;
    CBlockIndex randomXThenSha;

    const uint32_t secondTime =
        firstTime + consensus.nPowTargetSpacing;

    buildChild(
        shaThenRandomX,
        shaBranch1,
        ALGO_RANDOMX,
        secondTime);

    buildChild(
        randomXThenSha,
        randomXBranch1,
        ALGO_SHA256D,
        secondTime);

    // Each branch must accumulate exactly the normalized proof selected by
    // consensus, never the algorithm's raw native proof.
    BOOST_CHECK(
        shaThenRandomX.nChainWork ==
        shaBranch1.nChainWork +
        GetBlockProof(shaThenRandomX, consensus));

    BOOST_CHECK(
        randomXThenSha.nChainWork ==
        randomXBranch1.nChainWork +
        GetBlockProof(randomXThenSha, consensus));

    BOOST_CHECK(
        shaThenRandomX.nChainWork >
        shaBranch1.nChainWork);

    BOOST_CHECK(
        randomXThenSha.nChainWork >
        randomXBranch1.nChainWork);

    // Verify the per-algo links were propagated exactly like AddToBlockIndex.
    BOOST_CHECK(
        shaThenRandomX.lastAlgoBlocks[ALGO_RANDOMX] ==
        &shaThenRandomX);

    BOOST_CHECK(
        shaThenRandomX.lastAlgoBlocks[ALGO_SHA256D] ==
        &shaBranch1);

    BOOST_CHECK(
        randomXThenSha.lastAlgoBlocks[ALGO_SHA256D] ==
        &randomXThenSha);

    BOOST_CHECK(
        randomXThenSha.lastAlgoBlocks[ALGO_RANDOMX] ==
        &randomXBranch1);
}


BOOST_AUTO_TEST_CASE(MultiAlgo_mainnet_chainwork_scale_test)
{
    auto chainParams =
        CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    // Fixed historical mainnet SHA256D reference:
    // height 6985
    // hash 0000000000000055369a698dd1644e058afffd2a57b90ff5904eede0c35a0df1
    // nBits 196c92f2
    consensus.randomXActivationHeight = 6986;
    consensus.fPowNoRetargeting = true;
    consensus.multiAlgoChainworkScaleNumerator = 1921821664;
    consensus.multiAlgoChainworkScaleDenominator = 41733;

    CBlockIndex anchor;
    anchor.nHeight = 6985;
    anchor.nBits = 0x196c92f2;
    anchor.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;
    anchor.lastAlgoBlocks[ALGO_SHA256D] = &anchor;

    CBlockIndex child;
    child.pprev = &anchor;
    child.nHeight = 6986;
    child.nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

    const arith_uint256 referenceWork =
        GetBlockProof(anchor);
    const arith_uint256 scaledWork =
        GetBlockProof(child, consensus);

    BOOST_REQUIRE(referenceWork >= scaledWork);

    const arith_uint256 difference =
        referenceWork - scaledWork;

    BOOST_CHECK_EQUAL(difference.GetLow64(), 282U);
    BOOST_CHECK_EQUAL(difference.bits() <= 9, true);
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
        GetNextWorkRequired(&blocks[6498], nullptr, consensus, ALGO_SHA256D);
    BOOST_CHECK_EQUAL(preActivationBits, anchorBits);

    // Block 6500 is the first ASERT block. Ideal 10-minute spacing
    // must preserve the anchor target.
    const unsigned int idealBits =
        GetNextWorkRequired(&blocks[6499], nullptr, consensus, ALGO_SHA256D);
    BOOST_CHECK_EQUAL(idealBits, anchorBits);

    // Block 6501 after a very fast previous block: target must decrease
    // (difficulty increases).
    blocks[6500].nTime =
        blocks[6499].nTime + consensus.nPowTargetSpacing / 10;

    const unsigned int fastBits =
        GetNextWorkRequired(&blocks[6500], nullptr, consensus, ALGO_SHA256D);

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
        GetNextWorkRequired(&blocks[6500], nullptr, consensus, ALGO_SHA256D);

    arith_uint256 slowTarget;
    slowTarget.SetCompact(slowBits);

    BOOST_CHECK(slowTarget > anchorTarget);

    // Extreme delay must never produce a target above powLimit.
    blocks[6500].nTime =
        blocks[6499].nTime + consensus.asertHalfLife * 16;

    const unsigned int extremeSlowBits =
        GetNextWorkRequired(&blocks[6500], nullptr, consensus, ALGO_SHA256D);

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
        GetNextWorkRequired(&blocks[2], nullptr, consensus, ALGO_SHA256D);

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
        GetNextWorkRequired(&blocks[2], nullptr, consensus, ALGO_SHA256D),
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
        GetNextWorkRequired(&blocks[290], nullptr, consensus, ALGO_SHA256D),
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
            GetNextWorkRequired(&blocks[h], nullptr, consensus, ALGO_SHA256D),
            expected[h]);
    }
}


BOOST_AUTO_TEST_CASE(FastRise_activation_and_trigger_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 6500;
    consensus.fastRiseActivationHeight = 7000;

    std::vector<CBlockIndex> blocks(7001);

    arith_uint256 baseTarget = UintToArith256(consensus.powLimit);
    baseTarget >>= 8;
    const unsigned int baseBits = baseTarget.GetCompact();
    const int64_t startTime = 1800000000;

    // Build an otherwise perfectly spaced chain.
    for (int i = 0; i <= 7000; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = baseBits;
    }

    // Two fast intervals out of the last four must NOT trigger Fast-Rise.
    blocks[6995].nTime = startTime + 6995 * consensus.nPowTargetSpacing;
    blocks[6996].nTime = blocks[6995].nTime + 60;
    blocks[6997].nTime = blocks[6996].nTime + 60;
    blocks[6998].nTime = blocks[6997].nTime + 600;
    blocks[6999].nTime = blocks[6998].nTime + 600;

    auto baselineConsensus = consensus;
    baselineConsensus.fastRiseActivationHeight = 1000000;

    const unsigned int twoFastBaseline =
        GetNextWorkRequired(&blocks[6999], nullptr, baselineConsensus, ALGO_SHA256D);
    const unsigned int twoFastResult =
        GetNextWorkRequired(&blocks[6999], nullptr, consensus, ALGO_SHA256D);

    BOOST_CHECK_EQUAL(twoFastResult, twoFastBaseline);

    // Three fast intervals out of the last four MUST trigger Fast-Rise.
    blocks[6998].nTime = blocks[6997].nTime + 60;
    blocks[6999].nTime = blocks[6998].nTime + 600;

    const unsigned int threeFastBaseline =
        GetNextWorkRequired(&blocks[6999], nullptr, baselineConsensus, ALGO_SHA256D);
    const unsigned int threeFastResult =
        GetNextWorkRequired(&blocks[6999], nullptr, consensus, ALGO_SHA256D);

    arith_uint256 expectedFastTarget;
    expectedFastTarget.SetCompact(blocks[6999].nBits);
    expectedFastTarget >>= 1;

    BOOST_CHECK_NE(threeFastResult, threeFastBaseline);
    BOOST_CHECK_EQUAL(threeFastResult, expectedFastTarget.GetCompact());

    // The same pattern before activation must still behave exactly like ASERT.
    consensus.fastRiseActivationHeight = 7001;

    const unsigned int preActivationResult =
        GetNextWorkRequired(&blocks[6999], nullptr, consensus, ALGO_SHA256D);

    BOOST_CHECK_EQUAL(preActivationResult, threeFastBaseline);
}



BOOST_AUTO_TEST_CASE(FastRise_returns_to_ASERT_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 6500;
    consensus.fastRiseActivationHeight = 7000;

    std::vector<CBlockIndex> blocks(7002);

    arith_uint256 baseTarget = UintToArith256(consensus.powLimit);
    baseTarget >>= 8;
    const unsigned int baseBits = baseTarget.GetCompact();
    const int64_t startTime = 1800000000;

    for (int i = 0; i <= 7001; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = baseBits;
    }

    // Create a 3-of-4 fast pattern before block 7000.
    blocks[6995].nTime = startTime + 6995 * consensus.nPowTargetSpacing;
    blocks[6996].nTime = blocks[6995].nTime + 60;
    blocks[6997].nTime = blocks[6996].nTime + 60;
    blocks[6998].nTime = blocks[6997].nTime + 60;
    blocks[6999].nTime = blocks[6998].nTime + 600;

    const unsigned int triggeredBits =
        GetNextWorkRequired(&blocks[6999], nullptr, consensus, ALGO_SHA256D);

    arith_uint256 triggeredTarget;
    triggeredTarget.SetCompact(triggeredBits);

    arith_uint256 expectedTriggeredTarget;
    expectedTriggeredTarget.SetCompact(blocks[6999].nBits);
    expectedTriggeredTarget >>= 1;

    BOOST_CHECK_EQUAL(triggeredTarget.GetCompact(), expectedTriggeredTarget.GetCompact());

    // Simulate block 7000 being mined at the Fast-Rise target.
    blocks[7000].nBits = triggeredBits;
    blocks[7000].nTime = blocks[6999].nTime + 600;

    // The last four intervals are now 60, 60, 600, 600:
    // only two are fast, so Fast-Rise must stop immediately.
    auto baselineConsensus = consensus;
    baselineConsensus.fastRiseActivationHeight = 1000000;

    const unsigned int expectedASERT =
        GetNextWorkRequired(&blocks[7000], nullptr, baselineConsensus, ALGO_SHA256D);

    const unsigned int actual =
        GetNextWorkRequired(&blocks[7000], nullptr, consensus, ALGO_SHA256D);

    BOOST_CHECK_EQUAL(actual, expectedASERT);
}



BOOST_AUTO_TEST_CASE(FastRise_zero_and_negative_intervals_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.asertActivationHeight = 6500;
    consensus.fastRiseActivationHeight = 7000;

    std::vector<CBlockIndex> blocks(7001);

    arith_uint256 baseTarget = UintToArith256(consensus.powLimit);
    baseTarget >>= 8;
    const unsigned int baseBits = baseTarget.GetCompact();
    const int64_t startTime = 1800000000;

    for (int i = 0; i <= 7000; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = baseBits;
    }

    // Last four raw intervals:
    // 60, 0, -30, 600
    // Zero and negative intervals must count as fast.
    blocks[6995].nTime = startTime + 6995 * consensus.nPowTargetSpacing;
    blocks[6996].nTime = blocks[6995].nTime + 60;
    blocks[6997].nTime = blocks[6996].nTime;
    blocks[6998].nTime = blocks[6997].nTime - 30;
    blocks[6999].nTime = blocks[6998].nTime + 600;

    auto baselineConsensus = consensus;
    baselineConsensus.fastRiseActivationHeight = 1000000;

    const unsigned int baseline =
        GetNextWorkRequired(&blocks[6999], nullptr, baselineConsensus, ALGO_SHA256D);

    const unsigned int actual =
        GetNextWorkRequired(&blocks[6999], nullptr, consensus, ALGO_SHA256D);

    arith_uint256 expectedFastTarget;
    expectedFastTarget.SetCompact(blocks[6999].nBits);
    expectedFastTarget >>= 1;

    BOOST_CHECK_NE(actual, baseline);
    BOOST_CHECK_EQUAL(actual, expectedFastTarget.GetCompact());
}





BOOST_AUTO_TEST_CASE(MultiAlgo_DAA_activation_boundary_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    consensus.randomXActivationHeight = 40;

    std::vector<CBlockIndex> blocks(40);

    arith_uint256 baseTarget = UintToArith256(consensus.powLimit);
    baseTarget >>= 8;
    const unsigned int baseBits = baseTarget.GetCompact();
    const int64_t startTime = 1800000000;

    CBlockIndex* latest[NUM_ALGOS_IMPL]{};

    for (int i = 0; i < 40; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = baseBits;
        blocks[i].nVersion =
            BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

        for (int algo = 0; algo < NUM_ALGOS_IMPL; ++algo) {
            blocks[i].lastAlgoBlocks[algo] = latest[algo];
        }

        latest[ALGO_SHA256D] = &blocks[i];
    }

    // Height 39 -> next block is height 40, exactly the multi-algo
    // activation boundary.
    //
    // SHA256D is being requested directly after another SHA256D block,
    // so the local balancing rule must make it exactly one 4% step harder.
    const unsigned int firstMultiAlgoShaBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_SHA256D);

    arith_uint256 compactBaseTarget;
    compactBaseTarget.SetCompact(baseBits);

    arith_uint256 expectedShaTarget = compactBaseTarget;
    expectedShaTarget *= 100;
    expectedShaTarget /= 104;

    BOOST_CHECK_EQUAL(
        firstMultiAlgoShaBits,
        expectedShaTarget.GetCompact());

    // No RandomX block exists yet, so the first RandomX block must use
    // the explicitly configured RandomX bootstrap target.
    const unsigned int firstRandomXBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_RANDOMX);

    BOOST_CHECK_EQUAL(
        firstRandomXBits,
        RandomXInitialDifficulty(consensus));

    // Mainnet bootstrap calibration: approximately one RandomX block every
    // 20 minutes at the reference 66.76 kH/s hashrate.
    BOOST_CHECK_EQUAL(
        RandomXInitialDifficulty(consensus),
        0x1d359bc1U);
}


BOOST_AUTO_TEST_CASE(MultiAlgo_DAA_local_balance_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    // Test-only activation. Mainnet RandomX remains disabled in chainparams.
    consensus.randomXActivationHeight = 20;

    std::vector<CBlockIndex> blocks(40);

    arith_uint256 baseTarget = UintToArith256(consensus.powLimit);
    baseTarget >>= 8;
    const unsigned int baseBits = baseTarget.GetCompact();
    const int64_t startTime = 1800000000;

    for (int i = 0; i < 40; ++i) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = startTime + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = baseBits;

        // Before activation the chain is SHA256D-only.
        // From activation onward, build an ideal alternating sequence.
        if (i >= consensus.randomXActivationHeight && (i & 1)) {
            blocks[i].nVersion =
                BLOCK_VERSION_DEFAULT | BLOCK_VERSION_RANDOMX;
        } else {
            blocks[i].nVersion =
                BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;
        }
    }

    auto rebuildAlgoLinks = [&]() {
        CBlockIndex* latest[NUM_ALGOS_IMPL]{};

        for (auto& block : blocks) {
            for (int algo = 0; algo < NUM_ALGOS_IMPL; ++algo) {
                block.lastAlgoBlocks[algo] = latest[algo];
            }

            const int algo = block.GetAlgo();
            BOOST_REQUIRE(algo >= 0);
            BOOST_REQUIRE(algo < NUM_ALGOS_IMPL);
            latest[algo] = &block;
        }
    };

    rebuildAlgoLinks();

    // Height 39 is RandomX. Therefore SHA256D is the ideal next algo.
    // With perfect 10-minute global spacing its target must stay unchanged.
    const unsigned int balancedShaBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_SHA256D);

    BOOST_CHECK_EQUAL(balancedShaBits, baseBits);

    // RandomX was just mined at height 39. Mining RandomX again immediately
    // must make that algo 4% harder.
    const unsigned int repeatedRandomXBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_RANDOMX);

    arith_uint256 compactBaseTarget;
    compactBaseTarget.SetCompact(baseBits);

    arith_uint256 expectedRepeatedRandomXTarget = compactBaseTarget;
    expectedRepeatedRandomXTarget *= 100;
    expectedRepeatedRandomXTarget /= 104;

    BOOST_CHECK_EQUAL(
        repeatedRandomXBits,
        expectedRepeatedRandomXTarget.GetCompact());

    // Now simulate SHA256D also being mined at height 39, directly after
    // SHA256D at height 38. SHA must become harder while the missing RandomX
    // side becomes easier.
    blocks[39].nVersion =
        BLOCK_VERSION_DEFAULT | BLOCK_VERSION_SHA256D;

    rebuildAlgoLinks();

    const unsigned int repeatedShaBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_SHA256D);

    const unsigned int missingRandomXBits =
        GetNextWorkRequired(&blocks[39], nullptr, consensus, ALGO_RANDOMX);

    arith_uint256 expectedRepeatedShaTarget = compactBaseTarget;
    expectedRepeatedShaTarget *= 100;
    expectedRepeatedShaTarget /= 104;

    arith_uint256 expectedMissingRandomXTarget = compactBaseTarget;
    expectedMissingRandomXTarget *= 104;
    expectedMissingRandomXTarget /= 100;

    BOOST_CHECK_EQUAL(
        repeatedShaBits,
        expectedRepeatedShaTarget.GetCompact());

    BOOST_CHECK_EQUAL(
        missingRandomXBits,
        expectedMissingRandomXTarget.GetCompact());
}


BOOST_AUTO_TEST_CASE(RandomX_seed_height_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    auto consensus = chainParams->GetConsensus();

    BOOST_CHECK_EQUAL(consensus.randomXSeedEpochLength, 2048);
    BOOST_CHECK_EQUAL(consensus.randomXSeedLag, 64);

    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(0, consensus), 0);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(1, consensus), 0);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(64, consensus), 0);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(2047, consensus), 0);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(2048, consensus), 1984);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(2049, consensus), 1984);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(4095, consensus), 1984);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(4096, consensus), 4032);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(6144, consensus), 6080);

    consensus.randomXSeedEpochLength = 64;
    consensus.randomXSeedLag = 8;

    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(63, consensus), 0);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(64, consensus), 56);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(127, consensus), 56);
    BOOST_CHECK_EQUAL(GetRandomXSeedHeight(128, consensus), 120);
}


BOOST_AUTO_TEST_CASE(RandomX_seed_branch_test)
{
    auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::REGTEST);
    auto consensus = chainParams->GetConsensus();

    consensus.randomXSeedEpochLength = 64;
    consensus.randomXSeedLag = 8;

    // For a candidate block at height 128, the RandomX seed must come
    // from height 120 on the candidate's own branch.
    BOOST_REQUIRE_EQUAL(GetRandomXSeedHeight(128, consensus), 120);

    std::vector<CBlockIndex> mainBlocks(128);
    std::vector<uint256> mainHashes(128);

    for (int height = 0; height < 128; ++height) {
        mainBlocks[height].pprev =
            height ? &mainBlocks[height - 1] : nullptr;
        mainBlocks[height].nHeight = height;

        mainHashes[height] = GetRandHash();
        mainBlocks[height].phashBlock = &mainHashes[height];
        mainBlocks[height].BuildSkip();
    }

    // Fork after height 100, so height 120 differs between branches.
    static constexpr int forkHeight = 100;
    std::vector<CBlockIndex> sideBlocks(27);
    std::vector<uint256> sideHashes(27);

    for (int i = 0; i < 27; ++i) {
        const int height = forkHeight + 1 + i;

        sideBlocks[i].pprev =
            i ? &sideBlocks[i - 1] : &mainBlocks[forkHeight];
        sideBlocks[i].nHeight = height;

        sideHashes[i] = GetRandHash();
        sideBlocks[i].phashBlock = &sideHashes[i];
        sideBlocks[i].BuildSkip();
    }

    uint256 mainSeed;
    uint256 sideSeed;

    BOOST_REQUIRE(GetRandomXSeed(
        &mainBlocks[127], 128, consensus, mainSeed));
    BOOST_REQUIRE(GetRandomXSeed(
        &sideBlocks.back(), 128, consensus, sideSeed));

    BOOST_CHECK_EQUAL(mainSeed, mainHashes[120]);

    const int sideSeedIndex = 120 - (forkHeight + 1);
    BOOST_REQUIRE(sideSeedIndex >= 0);
    BOOST_REQUIRE(sideSeedIndex < static_cast<int>(sideHashes.size()));
    BOOST_CHECK_EQUAL(sideSeed, sideHashes[sideSeedIndex]);

    BOOST_CHECK(mainSeed != sideSeed);
}


BOOST_AUTO_TEST_CASE(RandomX_header_hash_vector_test)
{
    CBlockHeader header;
    header.nVersion = BLOCK_VERSION_DEFAULT | BLOCK_VERSION_RANDOMX;
    header.hashPrevBlock = uint256S("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    header.hashMerkleRoot = uint256S("f0e0d0c0b0a090807060504030201000ffeeddccbbaa99887766554433221100");
    header.nTime = 0x12345678;
    header.nBits = 0x1d00ffff;
    header.nNonce = 0x89abcdef;

    const uint256 seed =
        uint256S("00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f");

    CDataStream stream(SER_GETHASH, PROTOCOL_VERSION);
    stream << header;

    BOOST_REQUIRE_EQUAL(stream.size(), 80U);

    const std::vector<unsigned char> headerBytes(stream.begin(), stream.end());
    const std::vector<unsigned char> seedBytes(seed.begin(), seed.end());

    BOOST_CHECK_EQUAL(
        HexStr(headerBytes),
        "020400001f1e1d1c1b1a191817161514131211100f0e0d0c0b0a0908070605040302010000112233445566778899aabbccddeeff00102030405060708090a0b0c0d0e0f078563412ffff001defcdab89");

    BOOST_CHECK_EQUAL(
        HexStr(seedBytes),
        "0ffeeddccbbaa9988776655443322110ffeeddccbbaa99887766554433221100");

    uint256 randomXHash;
    BOOST_REQUIRE(header.GetRandomXPoWHash(seed, randomXHash));

    BOOST_CHECK_EQUAL(
        randomXHash.ToString(),
        "a694ce64be1245187466b2027afceb32c99b57bd21164dec2913b390f68c653c");

    const std::vector<unsigned char> hashBytes(randomXHash.begin(), randomXHash.end());

    BOOST_CHECK_EQUAL(
        HexStr(hashBytes),
        "3c658cf690b31329ec4d1621bd579bc932ebfc7a02b26674184512be64ce94a6");
}

BOOST_AUTO_TEST_SUITE_END()
