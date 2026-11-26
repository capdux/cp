// @cap/pathe — Zero-allocation universal POSIX-normalized path utilities
// Clean-room MIT implementation modeled on npm `pathe` (UnJS).

fn pthIsSep(b: u8): bool {
    return b == '/' || b == '\\';
}

fn pthIsAlpha(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
}

fn pthIsSchemeChar(b: u8): bool {
    return pthIsAlpha(b) || (b >= '0' && b <= '9') || b == '+' || b == '-' || b == '.';
}

fn pthHasDrivePrefix(s: string): bool {
    return s.len >= 2 && pthIsAlpha(s[0]) && s[1] == ':';
}

/// Returns length of `scheme://` prefix if present, else 0.
fn pthProtocolPrefixLen(s: string): usize {
    if (s.len < 4 || !pthIsAlpha(s[0])) return 0;
    let i: usize = 1;
    while (i < s.len && pthIsSchemeChar(s[i])) : (i += 1) {}
    if (i + 2 < s.len && s[i] == ':' && s[i + 1] == '/' && s[i + 2] == '/') {
        return i + 3;
    }
    return 0;
}

fn pthPush(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pthPushSlice(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        p = pthPush(out, p, s[i]);
    }
    return p;
}

fn pthStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Returns true if `path` is absolute (starts with `/`, `\`, `X:/`, `X:\`, or `scheme://`).
pub fn isAbsolute(path: string): bool {
    if (path.len == 0) return false;
    if (pthIsSep(path[0])) return true;
    if (pthHasDrivePrefix(path) && path.len >= 3 && pthIsSep(path[2])) return true;
    if (pthProtocolPrefixLen(path) > 0) return true;
    return false;
}

/// Normalize `path` to POSIX `/` separators, preserving Windows drive letters & protocol prefixes,
/// collapsing redundant slashes, and resolving `.` and `..` segments.
pub fn normalize(path: string, out: []u8): usize {
    if (path.len == 0) {
        return pthPush(out, 0, '.');
    }

    let pos: usize = 0;
    let rest_start: usize = 0;
    let is_abs = false;
    let has_protocol = false;

    const proto_len = pthProtocolPrefixLen(path);
    if (proto_len > 0) {
        pos = pthPushSlice(out, pos, path[0..proto_len]);
        rest_start = proto_len;
        is_abs = true;
        has_protocol = true;
    } else if (pthHasDrivePrefix(path)) {
        pos = pthPush(out, pos, path[0]);
        pos = pthPush(out, pos, ':');
        rest_start = 2;
        if (rest_start < path.len && pthIsSep(path[rest_start])) {
            is_abs = true;
            pos = pthPush(out, pos, '/');
            rest_start += 1;
        }
    } else if (pthIsSep(path[0])) {
        is_abs = true;
        pos = pthPush(out, pos, '/');
        rest_start = 1;
    }

    const trailing_sep = path.len > rest_start && pthIsSep(path[path.len - 1]);

    // Parse path segments into fixed stack (up to 128 segments)
    let seg_starts: [128]usize = undefined;
    let seg_lens: [128]usize = undefined;
    let seg_count: usize = 0;

    let i = rest_start;
    while (i < path.len) {
        while (i < path.len && pthIsSep(path[i])) : (i += 1) {}
        if (i >= path.len) break;
        const start = i;
        while (i < path.len && !pthIsSep(path[i])) : (i += 1) {}
        const slen = i - start;

        if (slen == 1 && path[start] == '.') {
            continue;
        }
        if (slen == 2 && path[start] == '.' && path[start + 1] == '.') {
            if (seg_count > 0) {
                const top_start = seg_starts[seg_count - 1];
                const top_len = seg_lens[seg_count - 1];
                const top_is_dotdot = top_len == 2 && path[top_start] == '.' && path[top_start + 1] == '.';
                if (!top_is_dotdot) {
                    seg_count -= 1;
                    continue;
                }
            }
            if (!is_abs && seg_count < 128) {
                seg_starts[seg_count] = start;
                seg_lens[seg_count] = slen;
                seg_count += 1;
            }
            continue;
        }
        if (seg_count < 128) {
            seg_starts[seg_count] = start;
            seg_lens[seg_count] = slen;
            seg_count += 1;
        }
    }

    if (seg_count == 0) {
        if (!is_abs && !has_protocol) {
            pos = pthPush(out, pos, '.');
            if (trailing_sep) {
                pos = pthPush(out, pos, '/');
            }
        }
        return pos;
    }

    let k: usize = 0;
    while (k < seg_count) : (k += 1) {
        if (k > 0) {
            pos = pthPush(out, pos, '/');
        }
        const s0 = seg_starts[k];
        const sl = seg_lens[k];
        pos = pthPushSlice(out, pos, path[s0..s0 + sl]);
    }
    if (trailing_sep) {
        pos = pthPush(out, pos, '/');
    }
    return pos;
}

/// Join two path segments and normalize to POSIX `/`.
pub fn join(a: string, b: string, out: []u8): usize {
    if (a.len == 0 && b.len == 0) {
        return pthPush(out, 0, '.');
    }
    if (a.len == 0) return normalize(b, out);
    if (b.len == 0) return normalize(a, out);

    let tmp: [4096]u8 = undefined;
    let tlen: usize = 0;
    tlen = pthPushSlice(tmp[0..], tlen, a);
    tlen = pthPush(tmp[0..], tlen, '/');
    tlen = pthPushSlice(tmp[0..], tlen, b);
    return normalize(tmp[0..tlen], out);
}

/// Alias for `join` that avoids shadowing CP's built-in concurrency `join(...)` primitive.
pub fn joinPath(a: string, b: string, out: []u8): usize {
    const f: *const fn (string, string, []u8) usize = join;
    return f(a, b, out);
}

fn pthStripTrailingSlash(out: []u8, len: usize): usize {
    if (len <= 1) return len;
    if (out[len - 1] != '/') return len;
    // Keep "C:/"
    if (len == 3 && pthIsAlpha(out[0]) && out[1] == ':' && out[2] == '/') return len;
    // Keep "scheme://"
    if (len >= 3 && out[len - 2] == '/' && out[len - 3] == ':') return len;
    return len - 1;
}

/// Resolve `rel` against `base` into a normalized POSIX path (strips non-root trailing `/`).
pub fn resolve(base: string, rel: string, out: []u8): usize {
    let n: usize = 0;
    if (rel.len == 0) {
        n = normalize(base, out);
    } else if (isAbsolute(rel)) {
        n = normalize(rel, out);
    } else if (base.len == 0) {
        n = normalize(rel, out);
    } else {
        let tmp: [4096]u8 = undefined;
        let tlen: usize = 0;
        tlen = pthPushSlice(tmp[0..], tlen, base);
        tlen = pthPush(tmp[0..], tlen, '/');
        tlen = pthPushSlice(tmp[0..], tlen, rel);
        n = normalize(tmp[0..tlen], out);
    }
    return pthStripTrailingSlash(out, n);
}

/// Return directory portion of `path` (handles both `/` and `\`).
pub fn dirname(path: string): string {
    if (path.len == 0) return ".";

    // Strip trailing separators
    let end = path.len;
    while (end > 0 && pthIsSep(path[end - 1])) : (end -= 1) {}
    if (end == 0) {
        return path[0..1];
    }

    // Handle bare drive root "C:" or "C:/"
    if (pthHasDrivePrefix(path) && end == 2) {
        if (path.len >= 3 && pthIsSep(path[2])) return path[0..3];
        return path[0..2];
    }

    // Find last separator before `end`
    let last_sep: usize = end;
    let i = end;
    while (i > 0) {
        i -= 1;
        if (pthIsSep(path[i])) {
            last_sep = i;
            break;
        }
    }

    if (last_sep == end) {
        if (pthHasDrivePrefix(path)) return path[0..2];
        return ".";
    }
    if (last_sep == 0) {
        return path[0..1];
    }
    if (pthHasDrivePrefix(path) && last_sep == 2) {
        return path[0..3];
    }

    // Trim redundant slashes before last_sep
    let trim_end = last_sep;
    while (trim_end > 1 && pthIsSep(path[trim_end - 1])) : (trim_end -= 1) {}
    return path[0..trim_end];
}

/// Return last path segment of `path` (handles both `/` and `\`).
pub fn basename(path: string): string {
    if (path.len == 0) return "";
    let end = path.len;
    while (end > 0 && pthIsSep(path[end - 1])) : (end -= 1) {}
    if (end == 0) return "";
    if (pthHasDrivePrefix(path) && end == 2) return "";

    let start: usize = 0;
    let i = end;
    while (i > 0) {
        i -= 1;
        if (pthIsSep(path[i])) {
            start = i + 1;
            break;
        }
    }
    if (start == 0 && pthHasDrivePrefix(path)) {
        start = 2;
    }
    return path[start..end];
}

/// Return file extension of `path` including leading `.`, or `""` if none.
pub fn extname(path: string): string {
    const base = basename(path);
    if (base.len <= 1) return "";
    if (base.len == 2 && base[0] == '.' && base[1] == '.') return "";

    let i = base.len;
    while (i > 0) {
        i -= 1;
        if (base[i] == '.') {
            if (i == 0) return "";
            return base[i..base.len];
        }
    }
    return "";
}

fn pthSplitSegments(s: string, starts: []usize, lens: []usize): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len && count < starts.len) {
        while (i < s.len && s[i] == '/') : (i += 1) {}
        if (i >= s.len) break;
        const start = i;
        while (i < s.len && s[i] != '/') : (i += 1) {}
        starts[count] = start;
        lens[count] = i - start;
        count += 1;
    }
    return count;
}

/// Compute relative POSIX path from `from_path` to `to_path`.
pub fn relative(from_path: string, to_path: string, out: []u8): usize {
    let fbuf: [4096]u8 = undefined;
    let tbuf: [4096]u8 = undefined;
    let flen = normalize(from_path, fbuf[0..]);
    let tlen = normalize(to_path, tbuf[0..]);
    flen = pthStripTrailingSlash(fbuf[0..], flen);
    tlen = pthStripTrailingSlash(tbuf[0..], tlen);

    const fstr: string = fbuf[0..flen];
    const tstr: string = tbuf[0..tlen];
    if (pthStrEq(fstr, tstr)) return 0;

    let f_starts: [128]usize = undefined;
    let f_lens: [128]usize = undefined;
    let t_starts: [128]usize = undefined;
    let t_lens: [128]usize = undefined;

    let f_off: usize = 0;
    let t_off: usize = 0;
    if (fstr.len == 1 && fstr[0] == '.') f_off = 1;
    if (tstr.len == 1 && tstr[0] == '.') t_off = 1;

    const f_cnt = pthSplitSegments(fstr[f_off..fstr.len], f_starts[0..], f_lens[0..]);
    const t_cnt = pthSplitSegments(tstr[t_off..tstr.len], t_starts[0..], t_lens[0..]);

    let common: usize = 0;
    while (common < f_cnt && common < t_cnt) : (common += 1) {
        const fs = fstr[f_off + f_starts[common]..f_off + f_starts[common] + f_lens[common]];
        const ts = tstr[t_off + t_starts[common]..t_off + t_starts[common] + t_lens[common]];
        if (!pthStrEq(fs, ts)) break;
    }

    let pos: usize = 0;
    let up = common;
    while (up < f_cnt) : (up += 1) {
        if (pos > 0) pos = pthPush(out, pos, '/');
        pos = pthPushSlice(out, pos, "..");
    }
    let dn = common;
    while (dn < t_cnt) : (dn += 1) {
        if (pos > 0) pos = pthPush(out, pos, '/');
        const seg = tstr[t_off + t_starts[dn]..t_off + t_starts[dn] + t_lens[dn]];
        pos = pthPushSlice(out, pos, seg);
    }
    return pos;
}
