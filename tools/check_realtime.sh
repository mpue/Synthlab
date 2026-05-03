#!/usr/bin/env bash
# Heuristik-Check fuer Regel A aus the-big-overhaul.md:
#   "Audio-Thread bleibt lock-free."
#
# Statt eines vollen LLVM-Aufrufgraphen (siehe Stufe 2 im Briefing) nutzen wir
# eine pragmatische Heuristik:
#
#   1) Bestimme die Funktionsrumpfe der konfigurierten Audio-Entry-Points.
#   2) Greppe darin nach den Symbolen aus tools/realtime_blacklist.txt.
#   3) Treffer = Verstoss, exit code != 0.
#
# Erlaubte Ausnahmen werden mit dem Inline-Marker `// RT-OK: <reason>` markiert
# und vom Check ueberlesen. Der Check zaehlt sie und gibt die Summe pro Lauf aus.
#
# Konfiguration:
#   tools/realtime_entry_points.txt   eine Regex pro Zeile (Funktionssignatur)
#   tools/realtime_blacklist.txt      eine Regex pro Zeile (verbotenes Symbol)
#
# Aufruf:
#   tools/check_realtime.sh           prueft Source/
#   tools/check_realtime.sh path/...  prueft den angegebenen Pfad
#
# Exit codes:
#   0 = sauber
#   1 = mind. eine Verletzung gefunden
#   2 = Konfigurationsfehler

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_ROOT="${1:-$ROOT/Source}"
TOOLS="$ROOT/tools"
ENTRY_FILE="$TOOLS/realtime_entry_points.txt"
BL_FILE="$TOOLS/realtime_blacklist.txt"
SCAN_FN="$TOOLS/lib/scan_function_body.sh"
EXCLUDES="$TOOLS/exclude_paths.txt"
BASELINE="$TOOLS/baseline.txt"

is_excluded() {
    local p="$1"
    [[ -f "$EXCLUDES" ]] || return 1
    while IFS= read -r pat; do
        [[ -z "$pat" || "$pat" =~ ^[[:space:]]*# ]] && continue
        if [[ "$p" == *"$pat"* ]]; then return 0; fi
    done < "$EXCLUDES"
    return 1
}

# Baseline lookup. Schluessel "RT:<rel_path>:<line>".
in_baseline() {
    local key="$1"
    [[ -f "$BASELINE" ]] || return 1
    grep -qE "^${key}([[:space:]]|$)" "$BASELINE"
}

for f in "$ENTRY_FILE" "$BL_FILE" "$SCAN_FN"; do
    if [[ ! -f "$f" ]]; then
        echo "[check_realtime] missing config: $f" >&2
        exit 2
    fi
done

violations=0
suppressions=0

# Lade Blacklist und baue ein einzelnes Regex.
BL_REGEX=""
while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    [[ "$line" =~ ^[[:space:]]*# ]] && continue
    if [[ -z "$BL_REGEX" ]]; then BL_REGEX="$line"; else BL_REGEX="${BL_REGEX}|${line}"; fi
done < "$BL_FILE"
BL_REGEX="(${BL_REGEX})"

while IFS= read -r src; do
    if is_excluded "$src"; then continue; fi
    while IFS= read -r entry_pattern; do
        [[ -z "$entry_pattern" ]] && continue
        [[ "$entry_pattern" =~ ^[[:space:]]*# ]] && continue

        while IFS=: read -r f start end; do
            [[ -z "$f" ]] && continue
            body=$(sed -n "${start},${end}p" "$f")
            sup_count=$(echo "$body" | grep -cE '// *RT-OK:' || true)
            suppressions=$((suppressions + sup_count))
            body_clean=$(echo "$body" | grep -vE '// *RT-OK:')

            while IFS= read -r hit_line; do
                [[ -z "$hit_line" ]] && continue
                rel=$(echo "$hit_line" | cut -d: -f1)
                content=$(echo "$hit_line" | cut -d: -f2-)
                abs=$((start + rel - 1))
                rel_path="${f#$ROOT/}"
                if in_baseline "RT:${rel_path}:${abs}"; then
                    continue
                fi
                echo "[RT-VIOLATION] ${rel_path}:${abs}"
                echo "  in body of: ${entry_pattern}"
                echo "  found:      ${content## }"
                violations=$((violations + 1))
            done < <(echo "$body_clean" | grep -nE "$BL_REGEX" || true)

        done < <(bash "$SCAN_FN" "$src" "$entry_pattern")
    done < "$ENTRY_FILE"
done < <(find "$SRC_ROOT" -type f \( -name '*.cpp' -o -name '*.h' \) | sort)

echo
echo "[check_realtime] suppressions seen: $suppressions"
if (( violations > 0 )); then
    echo "[check_realtime] violations: $violations" >&2
    exit 1
fi
echo "[check_realtime] clean."
exit 0
