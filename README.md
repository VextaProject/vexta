# Vexta Core

Vexta Core is the reference implementation of the Vexta blockchain.

Vexta is an open-source, UTXO-based proof-of-work cryptocurrency built on the Bitcoin Core architecture. It uses a single SHA256D mining algorithm, ten-minute target block times, and a Bitcoin-style subsidy halving schedule.

> **Early development software**
>
> Vexta Core v0.1.0 is an initial public release. Network parameters, software behavior, and compatibility requirements may still change. Back up wallets before testing and do not use funds you cannot afford to lose.

## Download

The latest Windows and Linux builds are available from the GitHub Releases page:

[Download Vexta Core v0.1.0](https://github.com/VextaProject/vexta/releases/tag/v0.1.0)

Available packages:

- Windows x86-64 portable ZIP
- Linux x86-64 portable TAR.GZ
- SHA-256 checksum files for both packages

Always verify the downloaded archive against its accompanying checksum file.

## Included Programs

| Program | Description |
|---|---|
| `vexta-qt` | Graphical wallet and full node |
| `vextad` | Command-line full node daemon |
| `vexta-cli` | RPC command-line client |
| `vexta-wallet` | Wallet maintenance utility |
| `vexta-tx` | Raw transaction utility |
| `vexta-util` | General Vexta utility |

## Network Parameters

- Name: Vexta
- Ticker: VTX
- Consensus: Proof of Work
- Mining algorithm: SHA256D
- Target block time: 10 minutes
- Initial block reward: 50 VTX
- Subsidy halving interval: 210,240 blocks
- SegWit: Enabled
- Taproot: Enabled
- URI scheme: `vexta:`

## Data Directory

Vexta Core uses a separate data directory and configuration from DigiByte Core.

The default configuration filename is `vexta.conf`.

## Building from Source

Build instructions are available in [INSTALL.md](INSTALL.md).

Typical Linux build process:

    ./autogen.sh
    ./configure
    make -j"$(nproc)"

Project dependencies may also be built using the included `depends` system.

## Testing

Run the unit tests with:

    make check

Functional tests are located in `test/functional`.

## Source Code

Official repository:

[github.com/VextaProject/vexta](https://github.com/VextaProject/vexta)

Release workflow documentation:

[RELEASE_WORKFLOW.md](RELEASE_WORKFLOW.md)

## Contributing

Contributions are welcome. Changes should prioritize security, stability, compatibility, maintainability, and careful review.

## License

Vexta Core is released under the MIT License. See [COPYING](COPYING) for details.

Vexta Core is derived from DigiByte Core and retains the applicable historical copyright and license notices.
