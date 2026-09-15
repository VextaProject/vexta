#!/usr/bin/env python3
# Copyright (c) 2026 The Vexta Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test activation-aware RandomX getblocktemplate handling."""

from test_framework.blocktools import NORMAL_GBT_REQUEST_PARAMS
from test_framework.test_framework import DigiByteTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


BLOCK_VERSION_ALGO = 15 << 8
BLOCK_VERSION_SHA256D = 2 << 8
BLOCK_VERSION_RANDOMX = 4 << 8


class RandomXGetBlockTemplateTest(DigiByteTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        self.log.info("RandomX GBT is rejected before activation")
        assert_raises_rpc_error(
            -8,
            "RandomX is not active yet",
            node.getblocktemplate,
            NORMAL_GBT_REQUEST_PARAMS,
            "randomx",
        )

        self.log.info("Default GBT remains SHA256D before activation")
        sha_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(sha_template["height"], 1)
        assert_equal(
            sha_template["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_SHA256D,
        )

        self.log.info("Activate RandomX at block 1 on regtest only")
        self.restart_node(0, extra_args=["-testactivationheight=randomx@1"])
        node = self.nodes[0]

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

        self.log.info("Default GBT remains SHA256D after RandomX activation")
        sha_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(sha_template["height"], 1)
        assert_equal(
            sha_template["version"] & BLOCK_VERSION_ALGO,
            BLOCK_VERSION_SHA256D,
        )


if __name__ == "__main__":
    RandomXGetBlockTemplateTest().main()
