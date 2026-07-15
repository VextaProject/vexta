#!/usr/bin/env python3
# Copyright (c) 2016 The DigiByte Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC commands for signing and verifying messages."""

from test_framework.test_framework import DigiByteTestFramework
from test_framework.util import assert_equal

class SignMessagesTest(DigiByteTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.nodes[0].createwallet("signmessages")
        wallet = self.nodes[0].get_wallet_rpc("signmessages")
        message = 'This is just a test message'

        self.log.info('skip signmessagewithprivkey without legacy wallet dumpprivkey support')

        self.log.info('test signing with an address with wallet')
        address = wallet.getnewaddress(address_type="legacy")
        signature = wallet.signmessage(address, message)
        assert(self.nodes[0].verifymessage(address, signature, message))

        self.log.info('test verifying with another address should not work')
        other_address = wallet.getnewaddress(address_type="legacy")
        other_signature = self.nodes[0].signmessage(other_address, message)
        assert(not self.nodes[0].verifymessage(other_address, signature, message))
        assert(not self.nodes[0].verifymessage(address, other_signature, message))

if __name__ == '__main__':
    SignMessagesTest().main()
