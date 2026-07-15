# Vexta VPS Deployment

## 1. Create Ubuntu server

Recommended:
- Ubuntu 24.04 LTS
- 2 vCPU minimum
- 4 GB RAM minimum
- 80 GB SSD

## 2. Update system

sudo apt update
sudo apt upgrade -y

## 3. Clone repository

git clone <repository-url>
cd vexta

## 4. Install Vexta

./contrib/vexta/install-node.sh

## 5. Configure node

mkdir -p ~/.vexta

cp contrib/vexta/vexta-mainnet.conf ~/.vexta/vexta.conf

Edit:

- rpcpassword
- addnode (optional)

## 6. Enable service

sudo systemctl enable vextad@$(whoami)

sudo systemctl start vextad@$(whoami)

## 7. Verify

systemctl status vextad@$(whoami)

vexta-cli getblockchaininfo

## 8. Firewall

Allow:

19333/tcp

RPC should remain localhost only.

## 9. Verify peers

vexta-cli getpeerinfo

vexta-cli getnetworkinfo

## 10. Verify sync

vexta-cli getblockchaininfo
