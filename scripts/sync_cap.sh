#!/usr/bin/env bash
# Atomic build & deploy script for `cap` (Zig toolchain) with compact error filtering.
# Usage:
#   bash scripts/sync_cap.sh           # Debug build + atomic deploy to /root/.cap/bin/cap
#   bash scripts/sync_cap.sh release   # ReleaseFast build + atomic deploy
#   bash scripts/sync_cap.sh fmt       # zig fmt cap/src + Debug build + atomic deploy
set -euo pipefail

ROOT="/data/dev/cp"
CAP_DIR="$ROOT/cap"
DEST="/root/.cap/bin/cap"
MODE="${1:-debug}"

OPT_FLAG=""
if [ "$MODE" = "release" ]; then
    OPT_FLAG="-Doptimize=ReleaseFast"
elif [ "$MODE" = "fmt" ]; then
    zig fmt "$CAP_DIR/build.zig" "$CAP_DIR/src" >/dev/null
fi

TMP_LOG="$(mktemp)"
trap 'rm -f "$TMP_LOG"' EXIT

T0=$(date +%s%3N)
if (cd "$CAP_DIR" && zig build $OPT_FLAG >"$TMP_LOG" 2>&1); then
    T1=$(date +%s%3N)
    mkdir -p "$(dirname "$DEST")"
    cp "$CAP_DIR/zig-out/cap" "${DEST}.new"
    mv "${DEST}.new" "$DEST"
    echo "[sync_cap] OK (${MODE}, $((T1 - T0))ms) -> $DEST"
else
    RC=$?
    # Compact error output: strip verbose `failed command:` and duplicate `Build Summary` tree
    grep -vE '^failed command:|^Build Summary:|^install transitive failure|^\+- install cap|^\|  \+- compile exe|^   \+- compile exe' "$TMP_LOG" | head -n 40 >&2
    exit "$RC"
fi
