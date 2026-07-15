#!/usr/bin/env bash
set -euo pipefail

DATADIR="${VEXTA_DATADIR:-$HOME/.vexta}"

vexta-cli -datadir="$DATADIR" stop
