#!/usr/bin/env bash
# Installiert den Pre-Commit-Hook fuer dieses Repo.
#   tools/install_hooks.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOOK="$ROOT/.git/hooks/pre-commit"
ln -sf "../../tools/pre-commit.sh" "$HOOK"
echo "Installed pre-commit hook -> $HOOK"
