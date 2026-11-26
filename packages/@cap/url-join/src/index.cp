// @cap/url-join — Zero-allocation smart URL joiner & path normalizer for CP
// Clean-room MIT implementation inspired by npm `url-join` and `ufo`.
//
// Features:
// - `urlJoin`: preserves schemes (`https://`, `http://`, `ws://`, `wss://`, `file:///`, `mailto:`, `//`),
//   deduplicates slashes, skips empty parts, merges multiple `?query` strings with `&`, and moves `#hash` to the end.
// - `normalizeUrlPath`: collapses redundant `/`, `.`, and `..` segments cleanly.
// - `withQuery`: appends `key=value` with automatic `?` vs `&` while preserving `#hash`.
// - `withTrailingSlash` & `withoutTrailingSlash`: query/hash-aware trailing slash helpers.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isAlpha(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isSchemeChar(c: u8) -> bool {
    return isAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
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
    if (s.len == 0 || !isAlpha(s[0])) {
        return 0;
    }
    let i: usize = 1;
    while (i < s.len && isSchemeChar(s[i])) {
        i += 1;
    }
    if (i < s.len && s[i] == ':') {
        if (i + 2 < s.len && s[i + 1] == '/' && s[i + 2] == '/') {
            // Preserve `file:///` triple slash when present
            if (i + 3 < s.len && s[i + 3] == '/' && i == 4 &&
                (s[0] == 'f' || s[0] == 'F') &&
                (s[1] == 'i' || s[1] == 'I') &&
                (s[2] == 'l' || s[2] == 'L') &&
                (s[3] == 'e' || s[3] == 'E')) {
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
    let hash_idx: usize = p.len;
    let i: usize = 0;
    while (i < p.len) {
        if (p[i] == '#') {
            hash_idx = i;
            break;
        }
        i += 1;
    }
    const before_hash = p[0..hash_idx];
    const hash = p[hash_idx..p.len];

    if (before_hash.len > 0 && before_hash[0] == '&') {
        return PartSplit{ path: "", query: before_hash[1..before_hash.len], hash: hash };
    }

    let q_idx: usize = before_hash.len;
    let j: usize = 0;
    while (j < before_hash.len) {
        if (before_hash[j] == '?') {
            q_idx = j;
            break;
        }
        j += 1;
    }

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
                pos = pushStr(out, pos, sp.path[0..pfx_len]);
                const rest = sp.path[pfx_len..sp.path.len];
                if (rest.len == 0) {
                    has_path = true;
                    ends_with_scheme = true;
                    last_path_had_trailing_slash = false;
                } else {
                    const trimmed = trimTrailingSlashes(rest);
                    pos = pushStr(out, pos, trimmed);
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
                pos = pushStr(out, pos, core);
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
            pos = pushStr(out, pos, core);
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
            pos = pushStr(out, pos, q);
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
        pos = pushStr(out, pos, final_hash);
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
        while (i < path.len && path[i] != '/') {
            i += 1;
        }
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
        pos = pushStr(out, pos, segs[k]);
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
    let hash_idx: usize = base_url.len;
    let i: usize = 0;
    while (i < base_url.len) {
        if (base_url[i] == '#') {
            hash_idx = i;
            break;
        }
        i += 1;
    }

    const before_hash = base_url[0..hash_idx];
    const hash_part = base_url[hash_idx..base_url.len];

    let has_q = false;
    let j: usize = 0;
    while (j < before_hash.len) {
        if (before_hash[j] == '?') {
            has_q = true;
            break;
        }
        j += 1;
    }

    let pos: usize = 0;
    pos = pushStr(out, pos, before_hash);
    if (!has_q) {
        pos = pushByte(out, pos, '?');
    } else if (before_hash.len > 0 && before_hash[before_hash.len - 1] != '?' && before_hash[before_hash.len - 1] != '&') {
        pos = pushByte(out, pos, '&');
    }
    pos = pushStr(out, pos, key);
    pos = pushByte(out, pos, '=');
    pos = pushStr(out, pos, value);
    pos = pushStr(out, pos, hash_part);

    return out[0..pos];
}

fn findQueryOrHash(url: string) -> usize {
    let i: usize = 0;
    while (i < url.len) {
        if (url[i] == '?' || url[i] == '#') {
            return i;
        }
        i += 1;
    }
    return url.len;
}

/// Ensures the path portion of `url` ends with `/`, preserving `?query` and `#hash`.
pub fn withTrailingSlash(url: string, out: []u8) -> string {
    const suf_idx = findQueryOrHash(url);
    const path_part = url[0..suf_idx];
    const suffix = url[suf_idx..url.len];

    let pos: usize = 0;
    pos = pushStr(out, pos, path_part);
    if (path_part.len == 0 || path_part[path_part.len - 1] != '/') {
        pos = pushByte(out, pos, '/');
    }
    pos = pushStr(out, pos, suffix);
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
    pos = pushStr(out, pos, path_part[0..end]);
    pos = pushStr(out, pos, suffix);
    return out[0..pos];
}
