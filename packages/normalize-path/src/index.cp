// @cap/normalize-path — Zero-heap POSIX/Windows path normalizer in pure CP
// MIT License — Clean-room implementation matching npm normalize-path semantics

fn npIsSlash(ch: u8): bool {
    return ch == 47 || ch == 92; // '/' or '\\'
}

fn npIsAsciiAlpha(ch: u8): bool {
    return (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122);
}

/// Return true if `path` contains any Windows backslash (`\`).
pub fn hasBackslash(path: string): bool {
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == 92) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Return true if `path` starts with a UNC or Windows extended-length device prefix
/// (`\\server\...`, `//server/...`, `\\?\...`, `\\.\...`).
/// Pure repeated root slashes like `//` or `///` return false.
pub fn isUncPath(path: string): bool {
    if (path.len < 3) {
        return false;
    }
    return npIsSlash(path[0]) && npIsSlash(path[1]) && !npIsSlash(path[2]);
}

/// Normalize path separators into `out`:
/// - Converts all Windows backslashes `\` to POSIX `/`
/// - Collapses consecutive slashes `//` into `/`, while preserving leading `//` for UNC & `\\?\` prefixes
/// - Strips a trailing `/` when `strip_trailing` is true, preserving root `/`, UNC `//`, and drive root `C:/`
/// - Safely truncates at `out.len` if the output buffer is smaller than the normalized path
/// Returns the number of bytes written to `out`.
pub fn normalizePath(path: string, strip_trailing: bool, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }

    const unc: bool = isUncPath(path);
    let w: usize = 0;
    let i: usize = 0;
    let prev_slash: bool = false;
    let truncated: bool = false;

    if (unc) {
        out[0] = 47;
        w = 1;
        if (out.len < 2) {
            return 1;
        }
        out[1] = 47;
        w = 2;
        i = 2;
        prev_slash = true;
    }

    while (i < path.len) {
        const ch: u8 = path[i];
        if (npIsSlash(ch)) {
            if (!prev_slash) {
                if (w >= out.len) {
                    truncated = true;
                    break;
                }
                out[w] = 47;
                w += 1;
                prev_slash = true;
            }
        } else {
            if (w >= out.len) {
                truncated = true;
                break;
            }
            out[w] = ch;
            w += 1;
            prev_slash = false;
        }
        i += 1;
    }

    if (!truncated && strip_trailing && w > 1 && out[w - 1] == 47) {
        const is_drive_root: bool = w == 3 && npIsAsciiAlpha(out[0]) && out[1] == 58;
        const is_unc_root: bool = unc && w == 2;
        if (!is_drive_root && !is_unc_root) {
            w -= 1;
        }
    }

    return w;
}

/// Normalize `path` with `strip_trailing = true` (default npm normalize-path behavior).
pub fn normalizePathDefault(path: string, out: []u8): usize {
    return normalizePath(path, true, out);
}

pub struct NormalizePathOptions {
    pub strip_trailing: bool,
    pub resolve_dots: bool,

    pub static fn defaults(): NormalizePathOptions {
        return NormalizePathOptions {
            strip_trailing: true,
            resolve_dots: false,
        };
    }
}

/// Return true if `path` starts with a Windows drive prefix such as `C:/`, `C:\`, or `C:`.
pub fn isWindowsDrivePath(path: string): bool {
    if (path.len < 2) {
        return false;
    }
    if (!npIsAsciiAlpha(path[0]) || path[1] != 58) {
        return false;
    }
    return path.len == 2 || npIsSlash(path[2]);
}

/// Convert all Windows backslashes `\` to POSIX `/` without collapsing slashes or stripping trailing slashes.
pub fn toPosixPath(path: string, out: []u8): usize {
    let w: usize = 0;
    while (w < path.len && w < out.len) {
        const ch = path[w];
        out[w] = if (ch == 92) 47 else ch;
        w += 1;
    }
    return w;
}

/// Zero-copy strip of trailing `/` or `\` characters while preserving root `/`, UNC `//`, and drive root `C:/`.
pub fn stripTrailingSlash(path: string): string {
    if (path.len <= 1) {
        return path;
    }
    const unc = isUncPath(path) || (path.len == 2 && npIsSlash(path[0]) && npIsSlash(path[1]));
    let end = path.len;
    while (end > 1 && npIsSlash(path[end - 1])) {
        const is_drive_root = end == 3 && npIsAsciiAlpha(path[0]) && path[1] == 58;
        const is_unc_root = unc && end == 2;
        if (is_drive_root || is_unc_root) {
            break;
        }
        end -= 1;
    }
    return path[0..end];
}

/// Normalize `path` with full `NormalizePathOptions` (`strip_trailing` and optional `.` / `..` segment resolution).
pub fn normalizePathWith(path: string, opts: NormalizePathOptions, out: []u8): usize {
    if (!opts.resolve_dots) {
        return normalizePath(path, opts.strip_trailing, out);
    }
    if (path.len == 0 || out.len == 0) {
        return 0;
    }

    let tmp: [512]u8 = undefined;
    const tmp_len = normalizePath(path, false, tmp[0..512]);
    if (tmp_len == 0) {
        return 0;
    }
    const norm: string = tmp[0..tmp_len] as string;
    const had_trailing = tmp_len > 1 && tmp[tmp_len - 1] == 47;

    // Detect prefix: UNC "//", drive "C:/", or POSIX root "/"
    let prefix_len: usize = 0;
    let is_abs: bool = false;
    if (isUncPath(norm)) {
        prefix_len = 2;
        is_abs = true;
    } else if (norm.len >= 3 && npIsAsciiAlpha(norm[0]) && norm[1] == 58 && norm[2] == 47) {
        prefix_len = 3;
        is_abs = true;
    } else if (norm[0] == 47) {
        prefix_len = 1;
        is_abs = true;
    }

    let seg_starts: [64]usize = undefined;
    let seg_lens: [64]usize = undefined;
    let seg_count: usize = 0;

    let i: usize = prefix_len;
    while (i <= norm.len) {
        if (i == norm.len || norm[i] == 47) {
            // Find start of current segment by scanning backward or tracking
            i += 1;
            continue;
        }
        const seg_s = i;
        while (i < norm.len && norm[i] != 47) {
            i += 1;
        }
        const seg_l = i - seg_s;
        if (seg_l == 1 && norm[seg_s] == 46) {
            // "." -> skip
        } else if (seg_l == 2 && norm[seg_s] == 46 && norm[seg_s + 1] == 46) {
            // ".." -> pop previous non-".." segment if possible
            if (seg_count > 0) {
                const prev_s = seg_starts[seg_count - 1];
                const prev_l = seg_lens[seg_count - 1];
                const prev_is_dotdot = prev_l == 2 && norm[prev_s] == 46 && norm[prev_s + 1] == 46;
                if (!prev_is_dotdot) {
                    seg_count -= 1;
                } else if (!is_abs && seg_count < 64) {
                    seg_starts[seg_count] = seg_s;
                    seg_lens[seg_count] = seg_l;
                    seg_count += 1;
                }
            } else if (!is_abs && seg_count < 64) {
                seg_starts[seg_count] = seg_s;
                seg_lens[seg_count] = seg_l;
                seg_count += 1;
            }
        } else if (seg_count < 64) {
            seg_starts[seg_count] = seg_s;
            seg_lens[seg_count] = seg_l;
            seg_count += 1;
        }
    }

    let w: usize = 0;
    let pi: usize = 0;
    while (pi < prefix_len && w < out.len) {
        out[w] = norm[pi];
        w += 1;
        pi += 1;
    }

    if (seg_count == 0 && !is_abs) {
        if (w < out.len) {
            out[w] = 46; // "."
            w += 1;
        }
        return w;
    }

    let si: usize = 0;
    while (si < seg_count) {
        if (si > 0 && w < out.len) {
            out[w] = 47;
            w += 1;
        }
        const ss = seg_starts[si];
        const sl = seg_lens[si];
        let k: usize = 0;
        while (k < sl && w < out.len) {
            out[w] = norm[ss + k];
            w += 1;
            k += 1;
        }
        si += 1;
    }

    if (!opts.strip_trailing && had_trailing && seg_count > 0 && w < out.len && out[w - 1] != 47) {
        out[w] = 47;
        w += 1;
    }

    return w;
}
