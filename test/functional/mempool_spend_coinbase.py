#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test spending coinbase transactions.

The coinbase transaction in block N can appear in block
N+100... so is valid in the mempool when the best block
height is N+99.
This test makes sure coinbase spends that will be mature
in the next block are accepted into the memory pool,
but less mature coinbase spends are NOT.
"""

from test_framework.test_framework import DigiByteTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.blocktools import COINBASE_MATURITY_2
from test_framework.wallet import MiniWallet, MiniWalletMode


class MempoolSpendCoinbaseTest(DigiByteTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)

        # Invalidate two blocks, so that miniwallet has access to a coin that will mature in the next block
        chain_height = 198
        wallet.generate(chain_height + 2, invalid_call=False)
        self.nodes[0].invalidateblock(self.nodes[0].getblockhash(chain_height + 1))
        assert_equal(chain_height, self.nodes[0].getblockcount())

        # Coinbase at height 99 is mature for mempool spend at height 198.
        # Coinbase at height 100 is still too immature.
        utxo_mature = wallet._utxos[chain_height - COINBASE_MATURITY_2]
        utxo_immature = wallet._utxos[chain_height - COINBASE_MATURITY_2 + 2]

        spend_mature_id = wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_mature)["txid"]

        # Vexta accepts the next coinbase spend into mempool as well.
        immature_tx = wallet.create_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_immature)
        spend_immature_id = self.nodes[0].sendrawtransaction(immature_tx['hex'])

        # mempool should have both spends
        assert_equal(set(self.nodes[0].getrawmempool()), {spend_mature_id, spend_immature_id})

        # mine a block, both should get confirmed
        self.generate(self.nodes[0], 1)
        assert_equal(set(self.nodes[0].getrawmempool()), set())


if __name__ == '__main__':
    MempoolSpendCoinbaseTest().main()
