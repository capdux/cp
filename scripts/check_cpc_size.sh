#!/usr/bin/env bash
set -euo pipefail

WARN_LIMIT="${WARN_LIMIT:-1800}"
HARD_LIMIT="${HARD_LIMIT:-2500}"

echo "=== cpc/src Rust File Size Report (warn>${WARN_LIMIT}, hard>${HARD_LIMIT}) ==="
OVER_HARD=0
OVER_WARN=0

while IFS= read -r line; do
    count=$(echo "$line" | awk '{print $1}')
    file=$(echo "$line" | awk '{print $2}')
    [ "$file" = "total" ] && continue
    if [ "$count" -gt "$HARD_LIMIT" ]; then
        printf "  [FAIL] %5d  %s\n" "$count" "$file"
        OVER_HARD=$((OVER_HARD + 1))
    elif [ "$count" -gt "$WARN_LIMIT" ]; then
        printf "  [WARN] %5d  %s\n" "$count" "$file"
        OVER_WARN=$((OVER_WARN + 1))
    fi
done < <(find cpc/src -name "*.rs" -print0 | xargs -0 wc -l | sort -rn)

echo "Top 10 largest files:"
find cpc/src -name "*.rs" -print0 | xargs -0 wc -l | grep -v ' total$' | sort -rn | head -n 10

if [ "$OVER_HARD" -gt 0 ]; then
    echo "[ERROR] ${OVER_HARD} file(s) exceed hard limit (${HARD_LIMIT} lines)."
    exit 1
fi
echo "[OK] 0 files exceed hard limit (${HARD_LIMIT} lines); ${OVER_WARN} soft warning(s)."
