#!/usr/bin/env bash
# Pre-commit hook: ruft die drei statischen Checks und blockiert bei Findings.
# Symlink/Kopie nach .git/hooks/pre-commit oder via core.hooksPath setzen.
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"

fail=0
"$ROOT/tools/check_realtime.sh"       || fail=1
"$ROOT/tools/check_save_isolation.sh" || fail=1
"$ROOT/tools/check_todos.sh"          || fail=1

if (( fail != 0 )); then
    echo
    echo "Pre-commit blocked by static checks." >&2
    echo "Fix the violations above, or document them in docs/the-big-overhaul-status.md" >&2
    echo "and add an inline suppression (// RT-OK / // SAVE-OK) with reason." >&2
    exit 1
fi
exit 0
