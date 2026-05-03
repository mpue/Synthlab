#!/usr/bin/env bash
# Regel C aus the-big-overhaul.md:
#   "TODO/FIXME/XXX/HACK ohne Issue-Referenz sind ein Fehler."
#
# Eine Issue-Referenz ist alles, was dem Pattern (#NNN) folgt.
#
# Aufruf: tools/check_todos.sh [pfad]
# Exit code: 0 sauber, 1 wenn Treffer.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_ROOT="${1:-$ROOT/Source}"
EXCLUDES="$ROOT/tools/exclude_paths.txt"
BASELINE="$ROOT/tools/baseline.txt"

is_excluded() {
    local p="$1"
    [[ -f "$EXCLUDES" ]] || return 1
    while IFS= read -r pat; do
        [[ -z "$pat" || "$pat" =~ ^[[:space:]]*# ]] && continue
        if [[ "$p" == *"$pat"* ]]; then return 0; fi
    done < "$EXCLUDES"
    return 1
}

in_baseline() {
    local key="$1"
    [[ -f "$BASELINE" ]] || return 1
    grep -qE "^${key}([[:space:]]|$)" "$BASELINE"
}

violations=0

while IFS= read -r src; do
    if is_excluded "$src"; then continue; fi
    while IFS= read -r line; do
        lineno=$(echo "$line" | cut -d: -f1)
        content=$(echo "$line" | cut -d: -f2-)
        if [[ "$content" =~ \(\#[0-9]+\) ]]; then
            continue
        fi
        rel_path="${src#$ROOT/}"
        if in_baseline "TODO:${rel_path}:${lineno}"; then
            continue
        fi
        echo "[TODO-NO-ISSUE] ${rel_path}:${lineno}"
        echo "  ${content## }"
        violations=$((violations + 1))
    done < <(grep -nE '\b(TODO|FIXME|XXX|HACK)\b' "$src" || true)
done < <(find "$SRC_ROOT" -type f \( -name '*.cpp' -o -name '*.h' \) | sort)

echo
if (( violations > 0 )); then
    echo "[check_todos] violations: $violations" >&2
    exit 1
fi
echo "[check_todos] clean."
exit 0
