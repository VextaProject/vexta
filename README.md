# Vexta Core

Vexta Core is the reference implementation of the Vexta blockchain.

## Overview

Vexta is a UTXO-based proof-of-work blockchain built on the Bitcoin Core architecture. It uses a single SHA256D mining algorithm, 10-minute block times, and a Bitcoin-style subsidy halving schedule.

## Network Parameters

- Consensus: Proof of Work (SHA256D)
- Target block time: 10 minutes
- Initial block reward: 50 VTX
- Subsidy halving interval: 210,240 blocks
- SegWit: Enabled
- Taproot: Enabled

## Building

Build instructions are available in [INSTALL.md](INSTALL.md).

Typical build process:

```bash
./autogen.sh
./configure
make -j$(nproc)
```

## Testing

Run the unit tests:

```bash
make check
```

Functional tests are located in `test/functional`.

## Contributing

Contributions are welcome. Changes should prioritize stability, compatibility, maintainability, and minimal divergence from upstream Bitcoin Core whenever practical.

## License

Vexta Core is released under the MIT License. See [COPYING](COPYING) for details.
