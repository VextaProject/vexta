# Vexta Core

Vexta Core is the reference implementation of the Vexta blockchain.

Vexta is an open-source, UTXO-based proof-of-work cryptocurrency built on the Bitcoin Core architecture. Vexta uses a multi-algorithm proof-of-work system combining **SHA256D** and **RandomX** on a single blockchain.

## Vexta Core v0.3.0

Vexta Core v0.3.0 introduces Vexta's multi-algorithm mining architecture.

RandomX activates on mainnet at **block 8,099**. SHA256D remains fully supported, with both algorithms sharing the same blockchain, transactions, addresses, block rewards, and consensus history.

Each mining algorithm maintains its own difficulty while contributing normalized chainwork to the same chain.

### Mining Algorithms

| Algorithm | Type | Status |
|---|---|---|
| SHA256D | ASIC-oriented proof of work | Active |
| RandomX | CPU-oriented proof of work | Activates at block 8,099 |

The network targets an average **10-minute global block interval** across both algorithms.

## Download

Official Windows and Linux builds are published on the GitHub Releases page:

https://github.com/VextaProject/Vexta/releases

Vexta Core v0.3.0 release packages:

- `vexta-v0.3.0-linux64.tar.gz`
- `vexta-v0.3.0-linux64-gui.tar.gz`
- `vexta-v0.3.0-win64.zip`

Always verify downloaded release files against the SHA-256 checksums published with the release.

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

- **Name:** Vexta
- **Ticker:** VTX
- **Consensus:** Proof of Work
- **Mining algorithms:** SHA256D + RandomX
- **RandomX mainnet activation:** Block 8,099
- **Target block time:** 10 minutes globally
- **Initial block reward:** 50 VTX
- **Subsidy halving interval:** 210,000 blocks
- **SegWit:** Enabled
- **Taproot:** Enabled
- **URI scheme:** `vexta:`
- **P2P port:** 19333
- **RPC port:** 19332

## Multi-Algorithm Mining

Vexta uses two independent proof-of-work algorithms on one blockchain.

SHA256D and RandomX blocks:

- share the same chain
- use the same transaction set and address system
- receive the same block subsidy rules
- maintain separate per-algorithm mining difficulty
- contribute normalized proof of work to the common chain

This allows different classes of mining hardware to participate without separating Vexta into independent chains.

## RandomX

RandomX is integrated as Vexta's second proof-of-work algorithm.

The implementation uses algorithm-aware block validation, independent RandomX difficulty tracking, deterministic RandomX seed selection, and normalized chainwork accounting.

Mainnet RandomX activation occurs at block **8,099**.

## Difficulty Adjustment

Vexta uses algorithm-aware difficulty handling so SHA256D and RandomX can respond independently to changes in mining power while remaining part of the same chain.

The network continues to target an average global block interval of approximately 10 minutes.

## Post-Quantum Development

Vexta Core also includes post-quantum wallet and address functionality developed as part of the project's longer-term cryptographic security work.

Multi-algorithm proof of work and post-quantum wallet functionality are separate parts of the Vexta architecture.

## Explorer

Official Vexta blockchain explorer:

https://vextaproject.org/explorer/

## Whitepaper

Official Vexta whitepaper:

https://vextaproject.org/whitepaper.pdf

## Mining

Official Vexta mining pool:

https://vexta-pool.co.uk

## Community

Discord:

https://discord.gg/zzpm7ghN3e

Official website:

https://vextaproject.org

## Data Directory

Vexta Core uses its own data directory and configuration.

The default configuration filename is:

```text
vexta.conf
```

## Building from Source

Build instructions are available in [INSTALL.md](INSTALL.md).

Typical Linux build process:

```bash
./autogen.sh
./configure
make -j"$(nproc)"
```

Project dependencies can also be built using the included `depends` system.

Vexta Core v0.3.0 additionally requires RandomX when building the multi-algorithm implementation.

## Testing

Unit tests can be built and run using the project test targets.

The Vexta v0.3.0 release was validated with targeted proof-of-work, blockchain, RandomX, multi-algorithm chainwork, difficulty, and block-validation tests.

Functional tests are located in:

```text
test/functional
```

## Source Code

Official repository:

https://github.com/VextaProject/Vexta

Release workflow documentation:

[RELEASE_WORKFLOW.md](RELEASE_WORKFLOW.md)

## Contributing

Contributions are welcome.

Changes should prioritize security, stability, compatibility, maintainability, and careful review.

Consensus changes should be reviewed especially carefully because they can affect compatibility with the live Vexta network.

## License

Vexta Core is released under the MIT License. See [COPYING](COPYING) for details.

Vexta Core is derived from DigiByte Core and retains the applicable historical copyright and license notices.
