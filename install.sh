#!/usr/bin/env bash
# CP Language Toolchain Production Installer (Linux & macOS)
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/cp-lang/cp/main/install.sh | bash
#
# Environment variables:
#   CP_VERSION         Release tag to install (default: latest, e.g. v1.0.0)
#   CP_INSTALL_DIR     Installation prefix (default: $HOME/.cap)
#   CP_REPO            GitHub repository slug (default: cp-lang/cp)
#   CP_NO_MODIFY_PATH  Set to 1 to skip shell profile PATH updates (default: 0)

set -euo pipefail

CP_VERSION="${CP_VERSION:-latest}"
CP_INSTALL_DIR="${CP_INSTALL_DIR:-$HOME/.cap}"
CP_REPO="${CP_REPO:-cp-lang/cp}"
CP_NO_MODIFY_PATH="${CP_NO_MODIFY_PATH:-0}"

info()  { printf '[cp-install] %s\n' "$*"; }
warn()  { printf '[cp-install] WARN: %s\n' "$*" >&2; }
error() { printf '[cp-install] ERROR: %s\n' "$*" >&2; exit 1; }

detect_os() {
    case "$(uname -s)" in
        Linux)  printf 'linux' ;;
        Darwin) printf 'macos' ;;
        *)      error "Unsupported OS: $(uname -s). Use install.ps1 on Windows." ;;
    esac
}

detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64)  printf 'x86_64' ;;
        aarch64|arm64) printf 'aarch64' ;;
        *)             error "Unsupported CPU architecture: $(uname -m)" ;;
    esac
}

fetch_file() {
    local url="$1"
    local dest="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fL --retry 3 --retry-delay 2 --silent --show-error "$url" -o "$dest"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$dest" "$url"
    else
        error "Neither 'curl' nor 'wget' is available. Please install one and retry."
    fi
}

fetch_text() {
    local url="$1"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL --retry 3 --retry-delay 2 "$url"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO- "$url"
    else
        error "Neither 'curl' nor 'wget' is available."
    fi
}

sha256_file() {
    local file="$1"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file" | awk '{print $1}'
    else
        error "Neither 'sha256sum' nor 'shasum' is available for SHA-256 verification."
    fi
}

resolve_version() {
    local ver="$1"
    if [ "$ver" = "latest" ]; then
        local api_url="https://api.github.com/repos/${CP_REPO}/releases/latest"
        local tag=""
        tag="$(fetch_text "$api_url" | sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -n 1)"
        if [ -z "$tag" ]; then
            error "Failed to resolve latest release tag from ${api_url}. Set CP_VERSION=v1.0.0 explicitly."
        fi
        printf '%s' "$tag"
    else
        case "$ver" in
            v*) printf '%s' "$ver" ;;
            *)  printf 'v%s' "$ver" ;;
        esac
    fi
}

verify_checksum() {
    local archive_path="$1"
    local archive_name="$2"
    local sums_path="$3"

    local expected actual
    expected="$(awk -v name="$archive_name" '$2 == name || $2 == ("*" name) { print tolower($1); exit }' "$sums_path")"
    if [ -z "$expected" ]; then
        error "Checksum entry for '${archive_name}' not found in SHA256SUMS.txt."
    fi

    actual="$(sha256_file "$archive_path" | tr '[:upper:]' '[:lower:]')"
    if [ "$expected" != "$actual" ]; then
        error "SHA-256 verification failed for ${archive_name}!
  expected: ${expected}
  actual:   ${actual}"
    fi
    info "SHA-256 verified (${actual:0:16}...)."
}

configure_path() {
    local bin_dir="$1"
    case ":${PATH}:" in
        *":${bin_dir}:"*)
            info "${bin_dir} is already in PATH."
            return 0
            ;;
    esac

    local export_line="export PATH=\"${bin_dir}:\$PATH\""
    if [ "$CP_NO_MODIFY_PATH" = "1" ]; then
        info "Skipped shell profile modification (CP_NO_MODIFY_PATH=1)."
        info "Add this line to your shell profile manually:"
        printf '  %s\n' "$export_line"
        return 0
    fi

    local updated=()
    local profile
    for profile in "$HOME/.bashrc" "$HOME/.zshrc"; do
        if [ -f "$profile" ]; then
            if ! grep -Fq "$bin_dir" "$profile" 2>/dev/null; then
                {
                    printf '\n# CP Language Toolchain\n'
                    printf '%s\n' "$export_line"
                } >> "$profile"
                updated+=("$profile")
            fi
        fi
    done

    if [ "${#updated[@]}" -gt 0 ]; then
        info "Added ${bin_dir} to PATH in: ${updated[*]}"
        info "Reload your shell or run: source ${updated[0]}"
    else
        warn "No ~/.bashrc or ~/.zshrc found. Add the following to your shell profile:"
        printf '  %s\n' "$export_line"
    fi
}

main() {
    command -v tar >/dev/null 2>&1 || error "'tar' is required to extract release archives."

    local os arch tag archive_name base_url tmp_dir
    os="$(detect_os)"
    arch="$(detect_arch)"
    tag="$(resolve_version "$CP_VERSION")"
    archive_name="cp-${tag}-${os}-${arch}.tar.gz"
    base_url="https://github.com/${CP_REPO}/releases/download/${tag}"

    tmp_dir="$(mktemp -d)"
    trap 'rm -rf "${tmp_dir:-}"' EXIT

    info "Installing CP ${tag} (${os}-${arch}) into ${CP_INSTALL_DIR}..."
    info "Downloading ${archive_name}..."
    fetch_file "${base_url}/${archive_name}" "${tmp_dir}/${archive_name}"
    info "Downloading SHA256SUMS.txt..."
    fetch_file "${base_url}/SHA256SUMS.txt" "${tmp_dir}/SHA256SUMS.txt"

    verify_checksum "${tmp_dir}/${archive_name}" "${archive_name}" "${tmp_dir}/SHA256SUMS.txt"

    mkdir -p "${CP_INSTALL_DIR}/bin" "${CP_INSTALL_DIR}/lib"
    local extract_dir="${tmp_dir}/extract"
    mkdir -p "$extract_dir"
    tar -xzf "${tmp_dir}/${archive_name}" -C "$extract_dir"

    [ -f "${extract_dir}/bin/cpc" ] || error "Release archive missing bin/cpc"
    [ -f "${extract_dir}/bin/cap" ] || error "Release archive missing bin/cap"

    install -m 755 "${extract_dir}/bin/cpc" "${CP_INSTALL_DIR}/bin/cpc"
    install -m 755 "${extract_dir}/bin/cap" "${CP_INSTALL_DIR}/bin/cap"

    if [ -d "${extract_dir}/lib" ]; then
        rm -rf "${CP_INSTALL_DIR}/lib"
        mv "${extract_dir}/lib" "${CP_INSTALL_DIR}/lib"
    fi

    local doc
    for doc in LICENSE-MIT LICENSE-APACHE THIRD_PARTY_NOTICES.md README.md; do
        if [ -f "${extract_dir}/${doc}" ]; then
            cp -f "${extract_dir}/${doc}" "${CP_INSTALL_DIR}/${doc}"
        fi
    done

    configure_path "${CP_INSTALL_DIR}/bin"

    info "Successfully installed CP ${tag}!"
    info "  cpc: ${CP_INSTALL_DIR}/bin/cpc"
    info "  cap: ${CP_INSTALL_DIR}/bin/cap"
    info "  lib: ${CP_INSTALL_DIR}/lib"
}

main "$@"
