// @cap/is-binary-path — Zero-heap binary/text path classifier & batch filter in pure CP
// MIT License — Composes @cap/binary-extensions directly with zero duplicated extension tables

import {
    CAT_NONE,
    CAT_IMAGE,
    CAT_AUDIO,
    CAT_VIDEO,
    CAT_ARCHIVE,
    CAT_FONT,
    CAT_EXECUTABLE,
    CAT_DOCUMENT,
    CAT_DATA_ML,
    isBinaryPath,
    isBinaryExtension,
    extensionCategory,
} from "../../binary-extensions/src/index.cp";

fn ibpExtractExt(path: string): string {
    if (path.len == 0) {
        return "";
    }
    let base_start: usize = 0;
    let i: usize = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == 47 || path[i] == 92) {
            base_start = i + 1;
            break;
        }
    }
    if (base_start >= path.len) {
        return "";
    }
    const base: string = path[base_start..path.len];
    let dot_idx: usize = 0;
    let found_dot: bool = false;
    let j: usize = base.len;
    while (j > 0) {
        j -= 1;
        if (base[j] == 46) {
            dot_idx = j;
            found_dot = true;
            break;
        }
    }
    if (!found_dot || dot_idx == 0 || dot_idx + 1 >= base.len) {
        return "";
    }
    return base[dot_idx + 1 .. base.len];
}

/// Return true if `path` is a non-empty non-binary path (including extensionless text files
/// like `Makefile`, `Dockerfile`, `README`, `LICENSE`, and dotfiles like `.gitignore`).
pub fn isTextPath(path: string): bool {
    if (path.len == 0) {
        return false;
    }
    return !isBinaryPath(path);
}

/// Collect indices of all binary paths in `paths` into `out_indices` (bounded by `out_indices.len`).
/// Returns the number of indices written.
pub fn filterBinaryPaths(paths: []const string, out_indices: []usize): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < paths.len && w < out_indices.len) {
        if (isBinaryPath(paths[i])) {
            out_indices[w] = i;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Collect indices of all text (non-binary, non-empty) paths in `paths` into `out_indices`.
/// Returns the number of indices written.
pub fn filterTextPaths(paths: []const string, out_indices: []usize): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < paths.len && w < out_indices.len) {
        if (isTextPath(paths[i])) {
            out_indices[w] = i;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Count how many paths in `paths` are binary paths.
pub fn countBinaryPaths(paths: []const string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < paths.len) {
        if (isBinaryPath(paths[i])) {
            total += 1;
        }
        i += 1;
    }
    return total;
}

/// Detect the binary category (`CAT_NONE`..`CAT_DATA_ML`) of `path` via `@cap/binary-extensions`.
pub fn detectPathKind(path: string): u8 {
    const ext: string = ibpExtractExt(path);
    if (ext.len == 0) {
        return CAT_NONE;
    }
    return extensionCategory(ext);
}

export {
    CAT_NONE,
    CAT_IMAGE,
    CAT_AUDIO,
    CAT_VIDEO,
    CAT_ARCHIVE,
    CAT_FONT,
    CAT_EXECUTABLE,
    CAT_DOCUMENT,
    CAT_DATA_ML,
    isBinaryPath,
    isBinaryExtension,
    extensionCategory,
    isTextPath,
    filterBinaryPaths,
    filterTextPaths,
    countBinaryPaths,
    detectPathKind,
};
