// @cap/url-join — Zero-allocation smart URL joiner & path normalizer for CP
// Clean-room MIT implementation inspired by npm `url-join` and `ufo`.
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

import { strEq, eqIgnoreCase } from "cap:ascii";

type UjVec16 = @Vector(16, u8);

const UJ_BIT_ALPHA: u8 = 1;
const UJ_BIT_SCHEME: u8 = 2;

fn ujBuildSchemeLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
            f |= UJ_BIT_ALPHA | UJ_BIT_SCHEME;
        } else if ((c >= 48 && c <= 57) || c == 43 || c == 45 || c == 46) {
            f |= UJ_BIT_SCHEME;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const UJ_SCHEME_LUT: [256]u8 = ujBuildSchemeLut();

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn ujCopySlice(out: []u8, pos: usize, src: string) -> usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (src.len < avail) { src.len } else { avail };
    @memcpy(out[pos .. pos + n], src[0..n]);
    return pos + n;
}

fn ujFindByte(s: string, start: usize, target: u8) -> usize {
    let i = start;
    const len = s.len;
    const v_t: UjVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: UjVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_t)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn isSchemeChar(c: u8) -> bool {
    return (UJ_SCHEME_LUT[c as usize] & UJ_BIT_SCHEME) != 0;
}

/// Returns the end index of a URL scheme prefix if present:
/// - `file:///` -> 8
/// - `https://` -> 8
/// - `mailto:` -> 7
/// - `//` (protocol-relative) -> 2
/// Returns 0 if no scheme prefix is present.
fn schemePrefixLen(s: string) -> usize {
    if (s.len >= 2 && s[0] == '/' && s[1] == '/') {
        return 2;
    }
    if (s.len == 0 || (UJ_SCHEME_LUT[s[0] as usize] & UJ_BIT_ALPHA) == 0) {
        return 0;
    }
    let i: usize = 1;
    while (i < s.len && isSchemeChar(s[i])) {
        i += 1;
    }
    if (i < s.len && s[i] == ':') {
        if (i + 2 < s.len && s[i + 1] == '/' && s[i + 2] == '/') {
            // Preserve `file:///` triple slash when present
            if (i + 3 < s.len && s[i + 3] == '/' && i == 4 && eqIgnoreCase(s[0..4], "file")) {
                return i + 4;
            }
            return i + 3;
        }
        return i + 1;
    }
    return 0;
}

struct PartSplit {
    path: string,
    query: string,
    hash: string,
}

fn splitPart(p: string) -> PartSplit {
    const hash_idx = ujFindByte(p, 0, '#');
    const before_hash = p[0..hash_idx];
    const hash = p[hash_idx..p.len];

    if (before_hash.len > 0 && before_hash[0] == '&') {
        return PartSplit{ path: "", query: before_hash[1..before_hash.len], hash: hash };
    }

    const q_idx = ujFindByte(before_hash, 0, '?');
    const path = before_hash[0..q_idx];
    const query = if (q_idx < before_hash.len) { before_hash[(q_idx + 1)..before_hash.len] } else { "" };
    return PartSplit{ path: path, query: query, hash: hash };
}

fn trimLeadingSlashes(s: string) -> string {
    let start: usize = 0;
    while (start < s.len && s[start] == '/') {
        start += 1;
    }
    return s[start..s.len];
}

fn trimTrailingSlashes(s: string) -> string {
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

/// Joins URL segments cleanly into caller-provided `out` (zero heap allocation).
/// Preserves URL schemes, deduplicates slashes, skips empty segments, merges multiple `?query`
/// strings with `&`, and places any `#hash` fragment at the end.
pub fn urlJoin(parts: []string, out: []u8) -> string {
    let pos: usize = 0;
    let has_path = false;
    let ends_with_scheme = false;
    let last_path_had_trailing_slash = false;

    // Pass 1: join path components
    let idx: usize = 0;
    while (idx < parts.len) {
        const raw = parts[idx];
        idx += 1;
        if (raw.len == 0) {
            continue;
        }
        const sp = splitPart(raw);
        if (sp.path.len == 0) {
            continue;
        }

        if (!has_path) {
            const pfx_len = schemePrefixLen(sp.path);
            if (pfx_len > 0) {
                pos = ujCopySlice(out, pos, sp.path[0..pfx_len]);
                const rest = sp.path[pfx_len..sp.path.len];
                if (rest.len == 0) {
                    has_path = true;
                    ends_with_scheme = true;
                    last_path_had_trailing_slash = false;
                } else {
                    const trimmed = trimTrailingSlashes(rest);
                    pos = ujCopySlice(out, pos, trimmed);
                    has_path = true;
                    ends_with_scheme = (trimmed.len == 0);
                    last_path_had_trailing_slash = (trimmed.len > 0 && rest[rest.len - 1] == '/');
                }
            } else {
                const leading_slash = (sp.path[0] == '/');
                const core = trimTrailingSlashes(trimLeadingSlashes(sp.path));
                if (leading_slash) {
                    pos = pushByte(out, pos, '/');
                }
                pos = ujCopySlice(out, pos, core);
                has_path = true;
                ends_with_scheme = (leading_slash && core.len == 0);
                last_path_had_trailing_slash = (core.len > 0 && sp.path[sp.path.len - 1] == '/');
            }
        } else {
            const core = trimTrailingSlashes(trimLeadingSlashes(sp.path));
            if (core.len == 0) {
                if (sp.path[sp.path.len - 1] == '/') {
                    last_path_had_trailing_slash = true;
                }
                continue;
            }
            if (!ends_with_scheme && pos > 0 && out[pos - 1] != '/') {
                pos = pushByte(out, pos, '/');
            }
            pos = ujCopySlice(out, pos, core);
            ends_with_scheme = false;
            last_path_had_trailing_slash = (sp.path[sp.path.len - 1] == '/');
        }
    }

    if (last_path_had_trailing_slash && pos > 0 && out[pos - 1] != '/') {
        pos = pushByte(out, pos, '/');
    }

    // Pass 2: merge all query components with '?' / '&'
    let first_query = true;
    idx = 0;
    while (idx < parts.len) {
        const raw = parts[idx];
        idx += 1;
        if (raw.len == 0) {
            continue;
        }
        const sp = splitPart(raw);
        let q = sp.query;
        while (q.len > 0 && (q[0] == '?' || q[0] == '&')) {
            q = q[1..q.len];
        }
        while (q.len > 0 && q[q.len - 1] == '&') {
            q = q[0..(q.len - 1)];
        }
        if (q.len > 0) {
            if (first_query) {
                pos = pushByte(out, pos, '?');
                first_query = false;
            } else {
                pos = pushByte(out, pos, '&');
            }
            pos = ujCopySlice(out, pos, q);
        }
    }

    // Pass 3: append final hash fragment (last one wins)
    let final_hash = "";
    idx = 0;
    while (idx < parts.len) {
        const raw = parts[idx];
        idx += 1;
        if (raw.len == 0) {
            continue;
        }
        const sp = splitPart(raw);
        if (sp.hash.len > 0) {
            final_hash = sp.hash;
        }
    }
    if (final_hash.len > 0) {
        pos = ujCopySlice(out, pos, final_hash);
    }

    return out[0..pos];
}

/// Normalizes a URL path by collapsing duplicate `/`, `.`, and `..` segments into `out`.
pub fn normalizeUrlPath(path: string, out: []u8) -> string {
    if (path.len == 0) {
        return out[0..0];
    }

    const is_abs = (path[0] == '/');
    const trailing_slash = (path.len > 1 && path[path.len - 1] == '/');

    let segs: [128]string = undefined;
    let seg_count: usize = 0;

    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && path[i] == '/') {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        i = ujFindByte(path, i, '/');
        const seg = path[start..i];

        if (strEq(seg, ".")) {
            continue;
        }
        if (strEq(seg, "..")) {
            if (seg_count > 0 && !strEq(segs[seg_count - 1], "..")) {
                seg_count -= 1;
            } else if (!is_abs && seg_count < 128) {
                segs[seg_count] = "..";
                seg_count += 1;
            }
            continue;
        }
        if (seg_count < 128) {
            segs[seg_count] = seg;
            seg_count += 1;
        }
    }

    let pos: usize = 0;
    if (is_abs) {
        pos = pushByte(out, pos, '/');
    }

    let k: usize = 0;
    while (k < seg_count) {
        if (k > 0) {
            pos = pushByte(out, pos, '/');
        }
        pos = ujCopySlice(out, pos, segs[k]);
        k += 1;
    }

    if (trailing_slash && seg_count > 0 && pos > 0 && out[pos - 1] != '/') {
        pos = pushByte(out, pos, '/');
    }

    return out[0..pos];
}

/// Appends a `key=value` query parameter to `base_url`, using `?` or `&` as appropriate
/// while preserving any trailing `#hash` fragment.
pub fn withQuery(base_url: string, key: string, value: string, out: []u8) -> string {
    const hash_idx = ujFindByte(base_url, 0, '#');
    const before_hash = base_url[0..hash_idx];
    const hash_part = base_url[hash_idx..base_url.len];
    const has_q = ujFindByte(before_hash, 0, '?') < before_hash.len;

    let pos: usize = 0;
    pos = ujCopySlice(out, pos, before_hash);
    if (!has_q) {
        pos = pushByte(out, pos, '?');
    } else if (before_hash.len > 0 && before_hash[before_hash.len - 1] != '?' && before_hash[before_hash.len - 1] != '&') {
        pos = pushByte(out, pos, '&');
    }
    pos = ujCopySlice(out, pos, key);
    pos = pushByte(out, pos, '=');
    pos = ujCopySlice(out, pos, value);
    pos = ujCopySlice(out, pos, hash_part);

    return out[0..pos];
}

fn findQueryOrHash(url: string) -> usize {
    let i: usize = 0;
    const len = url.len;
    const v_q: UjVec16 = @splat(63 as u8); // '?'
    const v_h: UjVec16 = @splat(35 as u8); // '#'
    while (i + 16 <= len) {
        const v: UjVec16 = url[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) || @reduce(.Or, v == v_h)) {
            let k: usize = 0;
            while (k < 16) {
                const c = url[i + k];
                if (c == '?' || c == '#') { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (url[i] == '?' || url[i] == '#') {
            return i;
        }
        i += 1;
    }
    return len;
}

/// Ensures the path portion of `url` ends with `/`, preserving `?query` and `#hash`.
pub fn withTrailingSlash(url: string, out: []u8) -> string {
    const suf_idx = findQueryOrHash(url);
    const path_part = url[0..suf_idx];
    const suffix = url[suf_idx..url.len];

    let pos: usize = 0;
    pos = ujCopySlice(out, pos, path_part);
    if (path_part.len == 0 || path_part[path_part.len - 1] != '/') {
        pos = pushByte(out, pos, '/');
    }
    pos = ujCopySlice(out, pos, suffix);
    return out[0..pos];
}

/// Removes trailing `/` from the path portion of `url` (while preserving root `/` and scheme prefixes),
/// keeping `?query` and `#hash` intact.
pub fn withoutTrailingSlash(url: string, out: []u8) -> string {
    const suf_idx = findQueryOrHash(url);
    const path_part = url[0..suf_idx];
    const suffix = url[suf_idx..url.len];

    let min_keep: usize = schemePrefixLen(path_part);
    if (min_keep == 0 && path_part.len > 0 && path_part[0] == '/') {
        min_keep = 1;
    }

    let end = path_part.len;
    while (end > min_keep && path_part[end - 1] == '/') {
        end -= 1;
    }

    let pos: usize = 0;
    pos = ujCopySlice(out, pos, path_part[0..end]);
    pos = ujCopySlice(out, pos, suffix);
    return out[0..pos];
}
