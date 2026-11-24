#!/usr/bin/env bash
# Package pre-built CP toolchain binaries (cpc + cap), standard library (lib/),
# and license notices into a distributable release tarball + SHA256SUMS.txt.
#
# Usage:
#   bash scripts/package_release.sh [version] [target]
#
# Defaults:
#   version = v1.0.0
#   target  = linux-x86_64
#   OUT_DIR = <repo>/temp/release_dist

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

CP_VERSION="${1:-${CP_VERSION:-v1.0.0}}"
CP_TARGET="${2:-${CP_TARGET:-linux-x86_64}}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/temp/release_dist}"

case "$CP_VERSION" in
    v*) ;;
    *)  CP_VERSION="v${CP_VERSION}" ;;
esac

find_binary() {
    local name="$1"
    shift
    local cand
    for cand in "$@"; do
        if [ -x "$cand" ]; then
            printf '%s' "$cand"
            return 0
        fi
    done
    printf '[package_release] ERROR: Could not locate compiled binary "%s".\n' "$name" >&2
    exit 1
}

CPC_BIN="$(find_binary cpc \
    "${ROOT_DIR}/cpc/target/release/cpc" \
    "${ROOT_DIR}/cap/bin/cpc" \
    "${HOME}/.cap/bin/cpc")"

CAP_BIN="$(find_binary cap \
    "${ROOT_DIR}/cap/zig-out/cap" \
    "${ROOT_DIR}/cap/bin/cap" \
    "${HOME}/.cap/bin/cap")"

for req in \
    "${ROOT_DIR}/lib/cap.json" \
    "${ROOT_DIR}/LICENSE-MIT" \
    "${ROOT_DIR}/LICENSE-APACHE" \
    "${ROOT_DIR}/THIRD_PARTY_NOTICES.md" \
    "${ROOT_DIR}/README.md"; do
    if [ ! -f "$req" ]; then
        printf '[package_release] ERROR: Missing required file: %s\n' "$req" >&2
        exit 1
    fi
done

ARCHIVE_NAME="cp-${CP_VERSION}-${CP_TARGET}.tar.gz"
ARCHIVE_PATH="${OUT_DIR}/${ARCHIVE_NAME}"
SUMS_PATH="${OUT_DIR}/SHA256SUMS.txt"

STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "${STAGE_DIR:-}"' EXIT

mkdir -p "${STAGE_DIR}/bin" "${STAGE_DIR}/lib" "${OUT_DIR}"

install -m 755 "$CPC_BIN" "${STAGE_DIR}/bin/cpc"
install -m 755 "$CAP_BIN" "${STAGE_DIR}/bin/cap"

tar -cf - \
    --exclude='*.a' \
    --exclude='*.so' \
    --exclude='*.dylib' \
    --exclude='*.dll' \
    --exclude='*.o' \
    --exclude='*.zcs' \
    --exclude='.cap' \
    --exclude='.zig-cache' \
    -C "${ROOT_DIR}/lib" . | tar -xf - -C "${STAGE_DIR}/lib"

cp -f \
    "${ROOT_DIR}/LICENSE-MIT" \
    "${ROOT_DIR}/LICENSE-APACHE" \
    "${ROOT_DIR}/THIRD_PARTY_NOTICES.md" \
    "${ROOT_DIR}/README.md" \
    "${STAGE_DIR}/"

tar -czf "$ARCHIVE_PATH" -C "$STAGE_DIR" .

(
    cd "$OUT_DIR"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$ARCHIVE_NAME" > SHA256SUMS.txt
    else
        shasum -a 256 "$ARCHIVE_NAME" > SHA256SUMS.txt
    fi
)

ARCHIVE_SIZE="$(du -h "$ARCHIVE_PATH" | awk '{print $1}')"
echo "[package_release] Packaged: ${ARCHIVE_PATH} (${ARCHIVE_SIZE})"
echo "[package_release] Checksums: ${SUMS_PATH}"
cat "$SUMS_PATH"
