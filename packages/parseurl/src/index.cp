// @cap/parseurl — Pure CP zero-allocation fast HTTP request URL splitter.
// Clean-room MIT implementation inspired by Express `parseurl` fastparse.
// Composes `@cap/querystring` for on-demand query parameter extraction & decoding.
// Accelerated with 16-lane @Vector(16, u8) SIMD delimiter/safety scanning, comptime 256-entry LUTs, and @memcpy.

import { pickQueryParam, unescapeQuery } from "../../querystring/src/index.cp";

type PurlVec16 = @Vector(16, u8);

const PURL_FLAG_ALPHA: u8 = 1;
const PURL_FLAG_SCHEME: u8 = 2;

fn purlBuildSchemeLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
            f |= PURL_FLAG_ALPHA | PURL_FLAG_SCHEME;
        } else if ((c >= 48 && c <= 57) || c == 0x2B || c == 0x2D || c == 0x2E) {
            f |= PURL_FLAG_SCHEME;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

fn purlBuildHexLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

const PURL_SCHEME_LUT: [256]u8 = purlBuildSchemeLut();
const PURL_HEX_LUT: [256]u8 = purlBuildHexLut();

fn purlFindByteSimd(s: string, target: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const vt: PurlVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: PurlVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn purlFindTwoBytesSimd(s: string, a: u8, b: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const va: PurlVec16 = @splat(a);
    const vb: PurlVec16 = @splat(b);
    while (i + 16 <= len) {
        const v: PurlVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == va) || @reduce(.Or, v == vb)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != a && s[i] != b) {
        i += 1;
    }
    return i;
}

// Detects an absolute-form URI prefix (`scheme://authority`) and returns the index
// where the origin-form path/query/hash begins, or 0 if not absolute-form.
fn purlFindOriginEnd(raw_url: string): usize {
    if (raw_url.len < 4 || (PURL_SCHEME_LUT[raw_url[0] as usize] & PURL_FLAG_ALPHA) == 0) {
        return 0;
    }
    let i: usize = 1;
    while (i < raw_url.len && (PURL_SCHEME_LUT[raw_url[i] as usize] & PURL_FLAG_SCHEME) != 0) {
        i += 1;
    }
    if (i + 2 >= raw_url.len) {
        return 0;
    }
    if (raw_url[i] != 0x3A || raw_url[i + 1] != 0x2F || raw_url[i + 2] != 0x2F) { // "://"
        return 0;
    }
    let j = i + 3;
    const len = raw_url.len;
    const v_slash: PurlVec16 = @splat(0x2F as u8);
    const v_q: PurlVec16 = @splat(0x3F as u8);
    const v_h: PurlVec16 = @splat(0x23 as u8);
    while (j + 16 <= len) {
        const v: PurlVec16 = raw_url[j .. j + 16][0..16].*;
        if (@reduce(.Or, v == v_slash) || @reduce(.Or, v == v_q) || @reduce(.Or, v == v_h)) {
            break;
        }
        j += 16;
    }
    while (j < len) {
        const b = raw_url[j];
        if (b == 0x2F || b == 0x3F || b == 0x23) { // '/', '?', '#'
            return j;
        }
        j += 1;
    }
    return len;
}

/// Zero-copy parsed HTTP request URL components (matching Express `parseurl`).
pub struct ParsedRequestUrl {
    pub href: string,
    pub path: string,
    pub pathname: string,
    pub search: string,
    pub query: string,
    pub hash: string,
    pub has_query: bool,

    pub static fn empty(): ParsedRequestUrl {
        return ParsedRequestUrl {
            href: "",
            path: "",
            pathname: "",
            search: "",
            query: "",
            hash: "",
            has_query: false
        };
    }
}

/// Single-pass O(N) zero-allocation HTTP request URL splitter.
/// Handles origin-form (`/api/v1/users?page=2&sort=asc#top`),
/// absolute-form (`http://host:8080/a/b?x=1`), and empty strings.
pub fn parseRequestUrl(raw_url: string): ParsedRequestUrl {
    if (raw_url.len == 0) {
        return ParsedRequestUrl.empty();
    }

    const origin_end = purlFindOriginEnd(raw_url);
    const has_origin = origin_end > 0;
    const start = origin_end;

    if (has_origin && start >= raw_url.len) {
        return ParsedRequestUrl {
            href: raw_url,
            path: "/",
            pathname: "/",
            search: "",
            query: "",
            hash: "",
            has_query: false
        };
    }

    let q_pos = raw_url.len;
    let h_pos = raw_url.len;
    const first_delim = purlFindTwoBytesSimd(raw_url, 0x3F, 0x23, start);
    if (first_delim < raw_url.len) {
        if (raw_url[first_delim] == 0x3F) { // '?'
            q_pos = first_delim;
            h_pos = purlFindByteSimd(raw_url, 0x23, first_delim + 1);
        } else { // '#'
            h_pos = first_delim;
        }
    }

    const path_end = h_pos;
    const hash_str = if (h_pos < raw_url.len) { raw_url[h_pos..raw_url.len] } else { "" };

    if (q_pos < path_end) {
        const p_name = if (q_pos == start && has_origin) { "/" } else { raw_url[start..q_pos] };
        return ParsedRequestUrl {
            href: raw_url,
            path: raw_url[start..path_end],
            pathname: p_name,
            search: raw_url[q_pos..path_end],
            query: raw_url[(q_pos + 1)..path_end],
            hash: hash_str,
            has_query: true
        };
    }

    const p_name = if (path_end == start && has_origin) { "/" } else { raw_url[start..path_end] };
    return ParsedRequestUrl {
        href: raw_url,
        path: p_name,
        pathname: p_name,
        search: "",
        query: "",
        hash: hash_str,
        has_query: false
    };
}

/// Percent-decodes an HTTP request `pathname` into `out`, preserving literal `+`
/// characters (unlike `application/x-www-form-urlencoded` query decoding).
pub fn decodePathname(pathname: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < pathname.len && pos < out.len) {
        const next_pct = purlFindByteSimd(pathname, 0x25, i);
        if (next_pct > i) {
            const avail = out.len - pos;
            const run = next_pct - i;
            const copy_len = if (run < avail) { run } else { avail };
            @memcpy(out[pos .. pos + copy_len], pathname[i .. i + copy_len]);
            pos += copy_len;
            i += copy_len;
            if (pos >= out.len) {
                break;
            }
        }
        if (i >= pathname.len) {
            break;
        }
        if (i + 2 < pathname.len) {
            const hi = PURL_HEX_LUT[pathname[i + 1] as usize];
            const lo = PURL_HEX_LUT[pathname[i + 2] as usize];
            if ((hi | lo) < 16) {
                out[pos] = ((hi << 4) | lo) as u8;
                pos += 1;
                i += 3;
                continue;
            }
        }
        out[pos] = pathname[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

/// Checks whether a request `pathname` is safe against null bytes (`\0` / `%00`),
/// backslashes (`\` / `%5C`), and parent-directory traversal (`..` / `%2E%2E` segments).
pub fn isSafeRequestPath(pathname: string): bool {
    if (pathname.len == 0) {
        return false;
    }

    let i: usize = 0;
    let seg_len: usize = 0;
    let seg_dots: usize = 0;
    const len = pathname.len;
    const v_pct: PurlVec16 = @splat(0x25 as u8);
    const v_nul: PurlVec16 = @splat(0x00 as u8);
    const v_bs: PurlVec16 = @splat(0x5C as u8);
    const v_slash: PurlVec16 = @splat(0x2F as u8);
    const v_dot: PurlVec16 = @splat(0x2E as u8);

    while (i < len) {
        if (i + 16 <= len) {
            const v: PurlVec16 = pathname[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_pct) &&
                !@reduce(.Or, v == v_nul) &&
                !@reduce(.Or, v == v_bs) &&
                !@reduce(.Or, v == v_slash) &&
                !@reduce(.Or, v == v_dot)) {
                seg_len += 16;
                i += 16;
                continue;
            }
        }

        let ch = pathname[i];
        let step: usize = 1;

        if (ch == 0x25 && i + 2 < len) { // '%'
            const hi = PURL_HEX_LUT[pathname[i + 1] as usize];
            const lo = PURL_HEX_LUT[pathname[i + 2] as usize];
            if ((hi | lo) < 16) {
                ch = ((hi << 4) | lo) as u8;
                step = 3;
            }
        }

        if (ch == 0x00 || ch == 0x5C) { // '\0' or '\'
            return false;
        }

        if (ch == 0x2F) { // '/'
            if (seg_len == 2 && seg_dots == 2) {
                return false;
            }
            seg_len = 0;
            seg_dots = 0;
        } else {
            seg_len += 1;
            if (ch == 0x2E) { // '.'
                seg_dots += 1;
            }
        }

        i += step;
    }

    if (seg_len == 2 && seg_dots == 2) {
        return false;
    }
    return true;
}

/// Extracts and decodes the first query parameter matching `key` from `raw_url`
/// by delegating to `@cap/querystring`. Returns bytes written or `null` if absent.
pub fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize? {
    const parsed = parseRequestUrl(raw_url);
    if (!parsed.has_query || parsed.query.len == 0) {
        return null;
    }
    return pickQueryParam(parsed.query, key, out);
}

/// Decodes a URL query component (converting `+` to space and `%XX` to bytes)
/// via `@cap/querystring.unescapeQuery`.
pub fn decodeQueryValue(raw: string, out: []u8): usize {
    return unescapeQuery(raw, out);
}

export {
    ParsedRequestUrl,
    parseRequestUrl,
    decodePathname,
    isSafeRequestPath,
    getRequestQueryParam,
    decodeQueryValue
};
