# Vexta Core

This directory contains helper scripts and configuration examples for running a Vexta node.

## Files

- vexta.conf.example
- vexta-mainnet.conf
- vexta-testnet.conf
- vexta-regtest.conf
- install-node.sh
- start-mainnet.sh
- stop-node.sh

## Running a local node

Copy one of the example configuration files into your data directory:

cp vexta-mainnet.conf ~/.vexta/vexta.conf

Start the node:

vextad -daemon

Stop the node:

vexta-cli stop

Check blockchain status:

vexta-cli getblockchaininfo

## Installing on Ubuntu

Run the installer as a normal user with sudo access:

    ./contrib/vexta/install-node.sh

The script installs build dependencies, builds Vexta Core without the GUI, and installs the command-line binaries into /usr/local/bin.
