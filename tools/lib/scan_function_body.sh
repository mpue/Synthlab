#!/usr/bin/env bash
# Helper: emits the line range [start_line, end_line] of the function body whose
# opening signature line matches the provided pattern in $2.
#
# Heuristic: starts at the matched line, finds the first '{', then balances
# braces until the matching '}'. Naive about strings/comments containing
# braces; in this codebase that has not been an issue.
#
# Usage: scan_function_body.sh <file> <regex_for_signature_line>
# Output: lines of the form  <file>:<start>:<end>  (one per match)

set -euo pipefail

file="$1"
pattern="$2"

[[ -f "$file" ]] || exit 0

awk -v pattern="$pattern" '
BEGIN { matchLine = 0; depth = 0; start = 0 }
{
    if (matchLine == 0) {
        if ($0 ~ pattern) {
            matchLine = NR
        }
    }
    if (matchLine > 0) {
        n = length($0)
        for (i = 1; i <= n; i++) {
            c = substr($0, i, 1)
            if (c == "{") {
                if (depth == 0) start = NR
                depth++
            } else if (c == "}") {
                depth--
                if (depth == 0 && start > 0) {
                    print FILENAME ":" start ":" NR
                    matchLine = 0
                    start = 0
                }
            }
        }
    }
}
' "$file"
