# Vexta Core

This directory contains helper scripts and configuration examples for running a Vexta node.

## Files

- vexta.conf.example
- vexta-mainnet.conf
- vexta-testnet.conf
- vexta-regtest.conf

## Running a local node

Copy one of the example configuration files into your data directory:

cp vexta-mainnet.conf ~/.vexta/vexta.conf

Start the node:

vextad -daemon

Stop the node:

vexta-cli stop

Check blockchain status:

vexta-cli getblockchaininfo
