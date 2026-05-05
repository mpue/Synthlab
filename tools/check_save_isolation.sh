#!/usr/bin/env bash
# Heuristik-Check fuer Regel B aus the-big-overhaul.md:
#   "Save-Pfad ist vom Live-State isoliert."
#
# Vorgehen:
#   1) Im Save-Pfad (Funktionen, deren Signatur ein konfiguriertes Pattern matcht)
#      darf nichts aus dem Live-State-Set referenziert werden.
#   2) currentSnapshot.load() darf genau einmal pro Save-Pfad aufgerufen werden
#      (oder das Aequivalent aus tools/save_isolation.config).
#
# Konfiguration:
#   tools/save_isolation.config
#       SAVE_FUNCTIONS=  (regex pro Zeile in dieser Datei zwischen "<save>" und "</save>")
#       LIVE_STATE=      (regex pro Zeile zwischen "<live>" und "</live>")
#       SNAPSHOT_LOAD=   (regex zwischen "<snapshot_load>" und "</snapshot_load>")
#
# Inline-Suppression: `// SAVE-OK: <reason>`. Werden gezaehlt.
#
# Aufruf:
#   tools/check_save_isolation.sh           prueft Source/

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_ROOT="${1:-$ROOT/Source}"
TOOLS="$ROOT/tools"
CONFIG="$TOOLS/save_isolation.config"
SCAN_FN="$TOOLS/lib/scan_function_body.sh"
EXCLUDES="$TOOLS/exclude_paths.txt"

is_excluded() {
    local p="$1"
    [[ -f "$EXCLUDES" ]] || return 1
    while IFS= read -r pat; do
        [[ -z "$pat" || "$pat" =~ ^[[:space:]]*# ]] && continue
        if [[ "$p" == *"$pat"* ]]; then return 0; fi
    done < "$EXCLUDES"
    return 1
}

if [[ ! -f "$CONFIG" ]]; then
    echo "[check_save_isolation] missing config: $CONFIG" >&2
    exit 2
fi

# Parser fuer die simplen <tag>...</tag>-Sektionen
extract_section() {
    local tag="$1"
    awk -v tag="$tag" '
        $0 ~ "<"tag">"  { inside=1; next }
        $0 ~ "</"tag">" { inside=0 }
        inside && $0 !~ /^[[:space:]]*(#|$)/ { print }
    ' "$CONFIG"
}

# Lade Sektionen in tmpfiles, weil bash 3 keine arrays-via-process-substitution gut kann.
SAVE_TMP=$(mktemp); LIVE_TMP=$(mktemp); SNAP_TMP=$(mktemp)
trap 'rm -f "$SAVE_TMP" "$LIVE_TMP" "$SNAP_TMP"' EXIT
extract_section "save"          > "$SAVE_TMP"
extract_section "live"          > "$LIVE_TMP"
extract_section "snapshot_load" > "$SNAP_TMP"

if [[ ! -s "$SAVE_TMP" ]]; then
    echo "[check_save_isolation] no SAVE_FUNCTIONS configured — nothing to check." >&2
    exit 0
fi

LIVE_REGEX=""
while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    if [[ -z "$LIVE_REGEX" ]]; then LIVE_REGEX="$line"; else LIVE_REGEX="${LIVE_REGEX}|${line}"; fi
done < "$LIVE_TMP"
[[ -n "$LIVE_REGEX" ]] && LIVE_REGEX="(${LIVE_REGEX})"

SNAP_REGEX=""
while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    if [[ -z "$SNAP_REGEX" ]]; then SNAP_REGEX="$line"; else SNAP_REGEX="${SNAP_REGEX}|${line}"; fi
done < "$SNAP_TMP"
[[ -n "$SNAP_REGEX" ]] && SNAP_REGEX="(${SNAP_REGEX})"

violations=0
suppressions=0

while IFS= read -r src; do
    if is_excluded "$src"; then continue; fi
    while IFS= read -r sig; do
        [[ -z "$sig" ]] && continue
        while IFS=: read -r f start end; do
            [[ -z "$f" ]] && continue
            body=$(sed -n "${start},${end}p" "$f")
            sup_count=$(echo "$body" | grep -cE '// *SAVE-OK:' || true)
            suppressions=$((suppressions + sup_count))
            body_clean=$(echo "$body" | grep -vE '// *SAVE-OK:')

            rel_path="${f#$ROOT/}"

            # Live-State-Treffer melden.
            if [[ -n "$LIVE_REGEX" ]]; then
                while IFS= read -r hit_line; do
                    [[ -z "$hit_line" ]] && continue
                    rel=$(echo "$hit_line" | cut -d: -f1)
                    content=$(echo "$hit_line" | cut -d: -f2-)
                    abs=$((start + rel - 1))
                    echo "[SAVE-ISOLATION] ${rel_path}:${abs}"
                    echo "  in body of: ${sig}"
                    echo "  reads live-state: ${content## }"
                    echo "  required: read via snapshot only"
                    violations=$((violations + 1))
                done < <(echo "$body_clean" | grep -nE "$LIVE_REGEX" || true)
            fi

            # Snapshot-Load-Anzahl pruefen.
            if [[ -n "$SNAP_REGEX" ]]; then
                load_count=$(echo "$body_clean" | grep -cE "$SNAP_REGEX" || true)
                if (( load_count > 1 )); then
                    echo "[SAVE-ISOLATION] ${rel_path}:${start}"
                    echo "  in body of: ${sig}"
                    echo "  multiple snapshot loads: ${load_count} (expected exactly 1)"
                    violations=$((violations + 1))
                fi
            fi
        done < <(bash "$SCAN_FN" "$src" "$sig")
    done < "$SAVE_TMP"
done < <(find "$SRC_ROOT" -type f \( -name '*.cpp' -o -name '*.h' \) | sort)

echo
echo "[check_save_isolation] suppressions seen: $suppressions"
if (( violations > 0 )); then
    echo "[check_save_isolation] violations: $violations" >&2
    exit 1
fi
echo "[check_save_isolation] clean."
exit 0
