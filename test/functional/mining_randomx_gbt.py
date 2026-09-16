#!/usr/bin/env python3
# Copyright (c) 2026 The Vexta Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test activation-aware RandomX mining RPC handling."""

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR
from test_framework.blocktools import NORMAL_GBT_REQUEST_PARAMS
from test_framework.test_framework import DigiByteTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


BLOCK_VERSION_ALGO = 15 << 8
BLOCK_VERSION_SHA256D = 2 << 8
BLOCK_VERSION_RANDOMX = 4 << 8


class RandomXGetBlockTemplateTest(DigiByteTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        # Keep node 1 isolated at genesis so RandomX blocks can later be
        # submitted to it explicitly through submitblock.
        self.stop_node(1)

        self.log.info("RandomX GBT is rejected before activation")
        assert_raises_rpc_error(
            -8,
            "RandomX is not active yet",
            node.getblocktemplate,
            NORMAL_GBT_REQUEST_PARAMS,
            "randomx",
        )

        self.log.info("RandomX generateblock is rejected before activation")
        assert_raises_rpc_error(
            -8,
            "RandomX is not active yet",
            node.generateblock,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            [],
            "randomx",
            invalid_call=False,
        )

        self.log.info("Default GBT remains SHA256D before activation")
        sha_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(sha_template["height"], 1)
        assert_equal(
            sha_template["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_SHA256D,
        )
        assert_equal(sha_template["pow_algo_id"], 0)
        assert_equal(sha_template["pow_algo"], "sha256d")
        assert "randomx_seed_height" not in sha_template
        assert "randomx_seed" not in sha_template

        self.log.info("Mining RPC reports only SHA256D before activation")
        mining_info = node.getmininginfo()
        assert_equal(mining_info["pow_algo_id"], 0)
        assert_equal(mining_info["pow_algo"], "sha256d")
        assert_equal(set(mining_info["difficulties"]), {"sha256d"})
        assert_equal(set(mining_info["networkhashesps"]), {"sha256d"})
        assert_equal(
            mining_info["difficulty"],
            mining_info["difficulties"]["sha256d"],
        )
        assert_equal(
            mining_info["networkhashps"],
            mining_info["networkhashesps"]["sha256d"],
        )

        self.log.info("getnetworkhashps accepts both known algorithms")
        assert_equal(node.getnetworkhashps(120, -1, "sha256d"), 0)
        assert_equal(node.getnetworkhashps(120, -1, "randomx"), 0)
        assert_raises_rpc_error(
            -8,
            "Unknown mining algorithm",
            node.getnetworkhashps,
            120,
            -1,
            "invalid",
        )

        self.log.info("Blockchain RPC reports only SHA256D before activation")
        difficulty_info = node.getdifficulty()
        assert_equal(set(difficulty_info["difficulties"]), {"sha256d"})

        blockchain_info = node.getblockchaininfo()
        assert_equal(set(blockchain_info["difficulties"]), {"sha256d"})

        self.log.info("Mining algorithm bits override -blockversion algo bits")
        block_version = 0x200000ff
        self.restart_node(0, extra_args=[f"-blockversion={block_version}"])
        node = self.nodes[0]

        sha_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        expected_sha_version = (
            (block_version & ~BLOCK_VERSION_ALGO) |
            BLOCK_VERSION_SHA256D
        )
        assert_equal(sha_template["version"], expected_sha_version)

        self.log.info("Activate RandomX at block 1 on regtest only")
        self.restart_node(0, extra_args=["-testactivationheight=randomx@1"])
        node = self.nodes[0]

        self.log.info("Blockchain RPC exposes RandomX exactly at activation height")
        difficulty_info = node.getdifficulty()
        assert_equal(
            set(difficulty_info["difficulties"]),
            {"sha256d", "randomx"},
        )

        blockchain_info = node.getblockchaininfo()
        assert_equal(
            set(blockchain_info["difficulties"]),
            {"sha256d", "randomx"},
        )

        self.log.info("RandomX GBT is available exactly at activation height")
        randomx_template = node.getblocktemplate(
            NORMAL_GBT_REQUEST_PARAMS,
            "randomx",
        )
        assert_equal(randomx_template["height"], 1)
        assert_equal(
            randomx_template["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_RANDOMX,
        )
        assert_equal(randomx_template["pow_algo_id"], 1)
        assert_equal(randomx_template["pow_algo"], "randomx")
        assert_equal(randomx_template["randomx_seed_height"], 0)

        genesis_hash = node.getblockhash(0)
        expected_randomx_seed = bytes.fromhex(genesis_hash)[::-1].hex()
        assert_equal(randomx_template["randomx_seed"], expected_randomx_seed)
        assert_equal(len(randomx_template["randomx_seed"]), 64)

        self.log.info("Default GBT remains SHA256D after RandomX activation")
        sha_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(sha_template["height"], 1)
        assert_equal(
            sha_template["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_SHA256D,
        )
        assert_equal(sha_template["pow_algo_id"], 0)
        assert_equal(sha_template["pow_algo"], "sha256d")
        assert "randomx_seed_height" not in sha_template
        assert "randomx_seed" not in sha_template

        self.log.info("Mining RPC exposes SHA256D and RandomX at activation")
        mining_info = node.getmininginfo()
        assert_equal(set(mining_info["difficulties"]), {"sha256d", "randomx"})
        assert_equal(
            set(mining_info["networkhashesps"]),
            {"sha256d", "randomx"},
        )
        assert mining_info["difficulties"]["sha256d"] > 0
        assert mining_info["difficulties"]["randomx"] > 0
        assert_equal(mining_info["networkhashesps"]["randomx"], 0)
        assert_equal(node.getnetworkhashps(120, -1, "randomx"), 0)

        self.log.info("Mine and accept a real RandomX block")
        randomx_blocks = self.generatetodescriptor(
            node,
            1,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            1000000,
            "randomx",
            sync_fun=self.no_op,
        )
        assert_equal(len(randomx_blocks), 1)
        assert_equal(node.getblockcount(), 1)

        mined_block = node.getblock(randomx_blocks[0])
        assert_equal(
            mined_block["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_RANDOMX,
        )
        assert_equal(mined_block["pow_algo_id"], 1)
        assert_equal(mined_block["pow_algo"], "randomx")
        assert mined_block["pow_hash"] is not None
        assert mined_block["pow_hash"] != mined_block["hash"]

        self.log.info("getblockheader exposes the same RandomX PoW metadata")
        mined_header = node.getblockheader(randomx_blocks[0])
        assert_equal(mined_header["pow_algo_id"], 1)
        assert_equal(mined_header["pow_algo"], "randomx")
        assert_equal(mined_header["pow_hash"], mined_block["pow_hash"])

        self.log.info("RandomX mining statistics remain separated from SHA256D")
        mining_info = node.getmininginfo()
        assert_equal(set(mining_info["difficulties"]), {"sha256d", "randomx"})
        assert_equal(
            set(mining_info["networkhashesps"]),
            {"sha256d", "randomx"},
        )

        self.log.info("Interleaved blocks do not mix per-algorithm hashrates")

        next_time = mined_block["time"] + 600
        node.setmocktime(next_time)
        self.generatetodescriptor(
            node,
            1,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            1000000,
            "sha256d",
            sync_fun=self.no_op,
        )

        next_time += 600
        node.setmocktime(next_time)
        self.generatetodescriptor(
            node,
            1,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            1000000,
            "randomx",
            sync_fun=self.no_op,
        )

        randomx_hashrate_before_sha = node.getnetworkhashps(
            120, -1, "randomx"
        )
        assert randomx_hashrate_before_sha > 0

        next_time += 600
        node.setmocktime(next_time)
        self.generatetodescriptor(
            node,
            1,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            1000000,
            "sha256d",
            sync_fun=self.no_op,
        )

        randomx_hashrate_after_sha = node.getnetworkhashps(
            120, -1, "randomx"
        )
        assert_equal(
            randomx_hashrate_after_sha,
            randomx_hashrate_before_sha,
        )

        sha_hashrate_before_randomx = node.getnetworkhashps(
            120, -1, "sha256d"
        )
        assert sha_hashrate_before_randomx > 0

        next_time += 600
        node.setmocktime(next_time)
        self.generatetodescriptor(
            node,
            1,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            1000000,
            "randomx",
            sync_fun=self.no_op,
        )

        sha_hashrate_after_randomx = node.getnetworkhashps(
            120, -1, "sha256d"
        )
        assert_equal(
            sha_hashrate_after_randomx,
            sha_hashrate_before_randomx,
        )

        assert_equal(node.getblockcount(), 5)

        self.log.info("Reindex and reload the RandomX block from disk")
        self.restart_node(
            0,
            extra_args=[
                "-testactivationheight=randomx@1",
                "-reindex",
            ],
        )
        node = self.nodes[0]

        assert_equal(node.getblockcount(), 5)
        reloaded_block = node.getblock(randomx_blocks[0])

        assert_equal(reloaded_block["hash"], randomx_blocks[0])
        assert_equal(reloaded_block["pow_algo_id"], 1)
        assert_equal(reloaded_block["pow_algo"], "randomx")
        assert_equal(reloaded_block["pow_hash"], mined_block["pow_hash"])

        self.log.info("generateblock mines a real RandomX block")
        generated = self.generateblock(
            node,
            ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            [],
            "randomx",
            sync_fun=self.no_op,
        )
        assert_equal(node.getblockcount(), 6)

        generated_block = node.getblock(generated["hash"])
        assert_equal(
            generated_block["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_RANDOMX,
        )
        assert_equal(generated_block["pow_algo_id"], 1)
        assert_equal(generated_block["pow_algo"], "randomx")
        assert generated_block["pow_hash"] is not None
        assert generated_block["pow_hash"] != generated_block["hash"]

        self.log.info("Reindex chainstate with RandomX blocks")
        self.restart_node(
            0,
            extra_args=[
                "-testactivationheight=randomx@1",
                "-reindex-chainstate",
            ],
        )
        node = self.nodes[0]

        assert_equal(node.getblockcount(), 6)

        chainstate_block = node.getblock(generated["hash"])
        assert_equal(chainstate_block["hash"], generated["hash"])
        assert_equal(chainstate_block["pow_algo_id"], 1)
        assert_equal(chainstate_block["pow_algo"], "randomx")
        assert_equal(
            chainstate_block["pow_hash"],
            generated_block["pow_hash"],
        )

        self.log.info("Submit the mixed SHA256D and RandomX chain to an isolated node")
        self.start_node(
            1,
            extra_args=["-testactivationheight=randomx@1"],
        )
        submit_node = self.nodes[1]

        assert_equal(submit_node.getblockcount(), 0)

        for height in range(1, 7):
            block_hash = node.getblockhash(height)
            raw_block = node.getblock(block_hash, 0)

            assert_equal(
                submit_node.submitblock(raw_block),
                None,
            )

            assert_equal(
                submit_node.getblockcount(),
                height,
            )

        submitted_tip_hash = submit_node.getblockhash(6)
        assert_equal(submitted_tip_hash, generated["hash"])

        submitted_tip = submit_node.getblock(submitted_tip_hash)
        assert_equal(submitted_tip["pow_algo_id"], 1)
        assert_equal(submitted_tip["pow_algo"], "randomx")
        assert_equal(
            submitted_tip["pow_hash"],
            generated_block["pow_hash"],
        )


if __name__ == "__main__":
    RandomXGetBlockTemplateTest().main()
