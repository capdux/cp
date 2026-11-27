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
