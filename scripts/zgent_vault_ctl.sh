#!/usr/bin/env bash

set -euo pipefail

runtime_root="${XDG_RUNTIME_DIR:-/tmp/zeta-vault-$(id -u)}"

exec zeta_vault_ctl \
  --socket "$runtime_root/zeta/zgent.sock" \
  "$@"
