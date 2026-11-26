// @cap/parseurl — Pure CP zero-allocation fast HTTP request URL splitter.
// Clean-room MIT implementation inspired by Express `parseurl` fastparse.
// Composes `@cap/querystring` for on-demand query parameter extraction & decoding.

import { pickQueryParam, unescapeQuery } from "../../querystring/src/index.cp";

fn purlIsAlpha(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

fn purlIsSchemeChar(b: u8): bool {
    if (purlIsAlpha(b)) { return true; }
    if (b >= 48 && b <= 57) { return true; } // 0-9
    return b == 0x2B || b == 0x2D || b == 0x2E; // '+', '-', '.'
}

fn purlHexVal(ch: u8): u8 {
    if (ch >= 48 && ch <= 57) { return ch - 48; }
    if (ch >= 65 && ch <= 70) { return ch - 55; }
    if (ch >= 97 && ch <= 102) { return ch - 87; }
    return 255;
}

// Detects an absolute-form URI prefix (`scheme://authority`) and returns the index
// where the origin-form path/query/hash begins, or 0 if not absolute-form.
fn purlFindOriginEnd(raw_url: string): usize {
    if (raw_url.len < 4 || !purlIsAlpha(raw_url[0])) {
        return 0;
    }
    let i: usize = 1;
    while (i < raw_url.len && purlIsSchemeChar(raw_url[i])) {
        i = i + 1;
    }
    if (i + 2 >= raw_url.len) {
        return 0;
    }
    if (raw_url[i] != 0x3A || raw_url[i + 1] != 0x2F || raw_url[i + 2] != 0x2F) { // "://"
        return 0;
    }
    let j = i + 3;
    while (j < raw_url.len) {
        let b = raw_url[j];
        if (b == 0x2F || b == 0x3F || b == 0x23) { // '/', '?', '#'
            return j;
        }
        j = j + 1;
    }
    return raw_url.len;
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

    let origin_end = purlFindOriginEnd(raw_url);
    let has_origin = origin_end > 0;
    let start = origin_end;

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
    let i = start;
    while (i < raw_url.len) {
        let b = raw_url[i];
        if (b == 0x3F && q_pos == raw_url.len) { // '?'
            q_pos = i;
        } else if (b == 0x23) { // '#'
            h_pos = i;
            break;
        }
        i = i + 1;
    }

    let path_end = h_pos;
    let hash_str = if (h_pos < raw_url.len) { raw_url[h_pos..raw_url.len] } else { "" };

    if (q_pos < path_end) {
        let p_name = if (q_pos == start && has_origin) { "/" } else { raw_url[start..q_pos] };
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

    let p_name = if (path_end == start && has_origin) { "/" } else { raw_url[start..path_end] };
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
        let b = pathname[i];
        if (b == 0x25 && i + 2 < pathname.len) { // '%'
            let hi = purlHexVal(pathname[i + 1]);
            let lo = purlHexVal(pathname[i + 2]);
            if (hi != 255 && lo != 255) {
                out[pos] = ((hi << 4) | lo) as u8;
                pos = pos + 1;
                i = i + 3;
                continue;
            }
        }
        out[pos] = b;
        pos = pos + 1;
        i = i + 1;
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

    while (i < pathname.len) {
        let ch = pathname[i];
        let step: usize = 1;

        if (ch == 0x25 && i + 2 < pathname.len) { // '%'
            let hi = purlHexVal(pathname[i + 1]);
            let lo = purlHexVal(pathname[i + 2]);
            if (hi != 255 && lo != 255) {
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
            seg_len = seg_len + 1;
            if (ch == 0x2E) { // '.'
                seg_dots = seg_dots + 1;
            }
        }

        i = i + step;
    }

    if (seg_len == 2 && seg_dots == 2) {
        return false;
    }
    return true;
}

/// Extracts and decodes the first query parameter matching `key` from `raw_url`
/// by delegating to `@cap/querystring`. Returns bytes written or `null` if absent.
pub fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize? {
    let parsed = parseRequestUrl(raw_url);
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
