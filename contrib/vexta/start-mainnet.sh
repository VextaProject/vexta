#!/usr/bin/env bash
set -euo pipefail

DATADIR="${VEXTA_DATADIR:-$HOME/.vexta}"

mkdir -p "$DATADIR"

if [ ! -f "$DATADIR/vexta.conf" ]; then
  cp "$(dirname "$0")/vexta-mainnet.conf" "$DATADIR/vexta.conf"
  echo "Created $DATADIR/vexta.conf"
  echo "Edit rpcpassword before using this node publicly."
fi

vextad -datadir="$DATADIR" -daemon
