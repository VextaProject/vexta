# Vexta Core v0.3.0

## Major Network Upgrade: Multi-Algorithm Mining

Vexta Core v0.3.0 introduces **RandomX** as a second proof-of-work algorithm alongside **SHA256D**.

RandomX activates on Vexta mainnet at **block 8,099**.

After activation:

- SHA256D remains fully supported
- RandomX becomes available as the second mining algorithm
- both algorithms mine the same Vexta blockchain
- both use the same transactions, addresses, and block reward rules
- each algorithm maintains its own mining difficulty
- both contribute to the same chain through normalized chainwork

The network continues to target an average global block interval of approximately **10 minutes**.

## Upgrade Notice

Node operators, mining pools, exchanges, explorers, and other infrastructure should upgrade to **Vexta Core v0.3.0** before block **8,099**.

Existing SHA256D mining remains supported after activation.

## RandomX

RandomX expands Vexta mining beyond SHA256D ASIC hardware and adds a CPU-oriented mining option directly to the same network.

RandomX proof-of-work is integrated into Vexta consensus while preserving the existing Vexta blockchain, transaction system, addresses, and block reward rules.

## Multi-Algorithm Difficulty

SHA256D and RandomX maintain separate mining difficulty histories so each algorithm can respond independently to changes in its own mining power.

Both algorithms remain part of the same best-chain selection process through normalized chainwork.

## Post-Quantum Functionality

Vexta Core v0.3.0 retains the project's existing post-quantum wallet and address functionality.

Post-quantum functionality and multi-algorithm proof-of-work are separate parts of the Vexta architecture.

## Official Links

Website:

https://vextaproject.org

Blockchain explorer:

https://vextaproject.org/explorer/

GitHub:

https://github.com/VextaProject/Vexta

Discord:

https://discord.gg/zzpm7ghN3e
