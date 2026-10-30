#!/usr/bin/env bash

set -euo pipefail

: "${HOME:?HOME must be set}"

runtime_root="${XDG_RUNTIME_DIR:-/tmp/zeta-vault-$(id -u)}"

exec zeta_vault_server \
  --vault "$HOME/.local/share/zeta/zgent/vault.bin" \
  --socket "$runtime_root/zeta/zgent.sock"
