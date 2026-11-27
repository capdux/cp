// @cap/path-clean — Zero-heap lexical path cleaner (crates.io:path-clean / Go filepath.Clean)
// Accelerated with 16-lane @Vector(16, u8) SIMD slash scanning, fast clean-path detection, and bulk @memcpy.

type PclVec16 = @Vector(16, u8);

fn pclFindSlash(s: string, start: usize): usize {
    let i = start;
    const n = s.len;
    const v_sl: PclVec16 = @splat('/' as u8);
    while (i + 16 <= n) {
        const v: PclVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == '/') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < n) {
        if (s[i] == '/') {
            return i;
        }
        i += 1;
    }
    return n;
}

fn pclByteAt(a: string, b: string, has_sep: bool, idx: usize): u8 {
    if (idx < a.len) {
        return a[idx];
    }
    if (has_sep) {
        if (idx == a.len) {
            return '/';
        }
        return b[idx - a.len - 1];
    }
    return b[idx - a.len];
}

fn pclCopySegment(out: []u8, w_in: usize, src: string, s_start: usize, s_end: usize): usize {
    const seg_len = s_end - s_start;
    if (w_in < out.len && seg_len > 0) {
        const avail = out.len - w_in;
        const copy_len = if (seg_len < avail) seg_len else avail;
        @memcpy(out[w_in .. w_in + copy_len], src[s_start .. s_start + copy_len]);
    }
    return w_in + seg_len;
}

fn pclCleanTwo(a: string, b: string, has_sep: bool, out: []u8): usize {
    const sep_len: usize = if (has_sep) 1 else 0;
    const n: usize = a.len + sep_len + b.len;
    if (n == 0) {
        if (out.len > 0) {
            out[0] = '.';
            return 1;
        }
        return 0;
    }

    const rooted: bool = pclByteAt(a, b, has_sep, 0) == '/';
    let w: usize = 0;
    let r: usize = 0;
    let dotdot: usize = 0;

    if (rooted) {
        if (out.len > 0) {
            out[0] = '/';
        }
        w = 1;
        r = 1;
        dotdot = 1;
    }

    const b_base = a.len + sep_len;

    while (r < n) {
        const ch = pclByteAt(a, b, has_sep, r);
        if (ch == '/') {
            // Collapse multiple slashes
            r += 1;
        } else if (ch == '.' && (r + 1 == n || pclByteAt(a, b, has_sep, r + 1) == '/')) {
            // Single '.' element
            r += 1;
        } else if (ch == '.' && r + 1 < n && pclByteAt(a, b, has_sep, r + 1) == '.' && (r + 2 == n || pclByteAt(a, b, has_sep, r + 2) == '/')) {
            // '..' element
            r += 2;
            if (w > dotdot) {
                // Backtrack to previous separator or dotdot boundary
                w -= 1;
                while (w > dotdot && (w >= out.len || out[w] != '/')) {
                    w -= 1;
                }
            } else if (!rooted) {
                // Preserve leading '..' for relative paths
                if (w > 0) {
                    if (w < out.len) {
                        out[w] = '/';
                    }
                    w += 1;
                }
                if (w < out.len) {
                    out[w] = '.';
                }
                w += 1;
                if (w < out.len) {
                    out[w] = '.';
                }
                w += 1;
                dotdot = w;
            }
            // For rooted paths at dotdot == 1, extra '..' is clamped at '/'
        } else {
            // Normal path segment
            if ((rooted && w != 1) || (!rooted && w != 0)) {
                if (w < out.len) {
                    out[w] = '/';
                }
                w += 1;
            }
            if (r < a.len) {
                const seg_end = pclFindSlash(a, r);
                w = pclCopySegment(out, w, a, r, seg_end);
                r = seg_end;
                // If !has_sep and segment ran to end of a, continue into b
                if (!has_sep && r == a.len && b.len > 0) {
                    const b_end = pclFindSlash(b, 0);
                    w = pclCopySegment(out, w, b, 0, b_end);
                    r = a.len + b_end;
                }
            } else if (r >= b_base) {
                const bi = r - b_base;
                const b_end = pclFindSlash(b, bi);
                w = pclCopySegment(out, w, b, bi, b_end);
                r = b_base + b_end;
            } else {
                // Separator position (only possible if sep != '/', unreachable here)
                r += 1;
            }
        }
    }

    if (w == 0) {
        if (out.len > 0) {
            out[0] = '.';
            return 1;
        }
        return 0;
    }

    if (w > out.len) {
        return out.len;
    }
    return w;
}

/// Lexically clean a POSIX path following Go `filepath.Clean` / Rust `path-clean` rules.
/// Writes the cleaned path into `out` and returns the number of bytes written.
pub fn cleanPath(path: string, out: []u8): usize {
    if (isClean(path)) {
        const copy_len = if (path.len < out.len) path.len else out.len;
        if (copy_len > 0) {
            @memcpy(out[0..copy_len], path[0..copy_len]);
        }
        return copy_len;
    }
    return pclCleanTwo(path, "", false, out);
}

/// Join two path segments and lexically clean the combined result into `out`.
/// Returns the number of bytes written.
pub fn cleanJoin(a: string, b: string, out: []u8): usize {
    if (a.len == 0) {
        return cleanPath(b, out);
    }
    if (b.len == 0) {
        return cleanPath(a, out);
    }
    return pclCleanTwo(a, b, true, out);
}

/// Return true if `path` is already in canonical cleaned form (`cleanPath(path) == path`).
pub fn isClean(path: string): bool {
    const n = path.len;
    if (n == 0) {
        return false;
    }
    if (n == 1) {
        return true;
    }
    const rooted = path[0] == '/';
    // Trailing slash is never clean unless path is "/"
    if (path[n - 1] == '/') {
        return false;
    }

    let i: usize = if (rooted) 1 else 0;
    let seen_normal: bool = false;

    while (i < n) {
        // Consecutive slashes
        if (path[i] == '/') {
            return false;
        }
        const seg_start = i;
        i = pclFindSlash(path, seg_start);
        const seg_len = i - seg_start;

        if (seg_len == 1 && path[seg_start] == '.') {
            // Standalone "." only valid as the entire path (handled by n == 1 above)
            return false;
        }
        if (seg_len == 2 && path[seg_start] == '.' && path[seg_start + 1] == '.') {
            if (rooted || seen_normal) {
                return false;
            }
        } else {
            seen_normal = true;
        }

        if (i < n && path[i] == '/') {
            i += 1;
        }
    }

    return true;
}
