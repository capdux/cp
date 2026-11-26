// @cap/parseurl — Zero-heap zero-copy HTTP request URL parser in pure CP
// MIT License — Clean-room implementation inspired by npm `parseurl`

pub struct ParsedUrl {
    pub path_start: usize,
    pub path_len: usize,
    pub pathname_start: usize,
    pub pathname_len: usize,
    pub search_start: usize,
    pub search_len: usize,
    pub query_start: usize,
    pub query_len: usize,
    pub has_query: bool,
    pub has_scheme: bool,
    pub valid: bool,
}

fn invalidUrl(): ParsedUrl {
    return ParsedUrl {
        path_start: 0,
        path_len: 0,
        pathname_start: 0,
        pathname_len: 0,
        search_start: 0,
        search_len: 0,
        query_start: 0,
        query_len: 0,
        has_query: false,
        has_scheme: false,
        valid: false,
    };
}

fn isAsciiAlpha(ch: u8): bool {
    return (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122);
}

fn isSchemeChar(ch: u8): bool {
    return isAsciiAlpha(ch) || (ch >= 48 && ch <= 57) || ch == 43 || ch == 45 || ch == 46; // + - .
}

fn hasControlChar(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        const ch: u8 = s[i];
        if (ch < 0x20 || ch == 0x7F) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Detect `scheme://` at start of `url`. Returns index right after `://`, or `0` if none.
fn detectSchemeEnd(url: string): usize {
    if (url.len < 4 || !isAsciiAlpha(url[0])) {
        return 0;
    }
    let i: usize = 1;
    while (i < url.len && isSchemeChar(url[i])) {
        i += 1;
    }
    if (i + 2 < url.len && url[i] == 58 && url[i + 1] == 47 && url[i + 2] == 47) { // "://"
        return i + 3;
    }
    return 0;
}

/// Zero-copy HTTP request URL parser (relative request-target or full absolute URI).
/// Extracts spans for `path` (pathname+search), `pathname`, `search` (with `?`), and `query` (without `?`).
/// Rejects empty strings and ASCII control characters (`< 0x20` or `0x7F`).
pub fn parseUrl(url: string): ParsedUrl {
    if (url.len == 0 || hasControlChar(url)) {
        return invalidUrl();
    }

    let base_start: usize = 0;
    let has_scheme: bool = false;

    // Fast path: relative URI starting with '/' skips scheme detection entirely
    if (url[0] != 47) {
        const after_scheme: usize = detectSchemeEnd(url);
        if (after_scheme > 0) {
            has_scheme = true;
            let a: usize = after_scheme;
            while (a < url.len && url[a] != 47 && url[a] != 63 && url[a] != 35) {
                a += 1;
            }
            base_start = a;
        }
    }

    let hash_pos: usize = url.len;
    let q_pos: usize = url.len;
    let i: usize = base_start;
    while (i < url.len) {
        const ch: u8 = url[i];
        if (ch == 35) { // '#'
            hash_pos = i;
            break;
        }
        if (ch == 63 && q_pos == url.len) { // first '?'
            q_pos = i;
        }
        i += 1;
    }

    const effective_q: usize = if (q_pos < hash_pos) q_pos else hash_pos;
    const has_query: bool = q_pos < hash_pos;

    return ParsedUrl {
        path_start: base_start,
        path_len: hash_pos - base_start,
        pathname_start: base_start,
        pathname_len: effective_q - base_start,
        search_start: effective_q,
        search_len: if (has_query) hash_pos - effective_q else 0,
        query_start: if (has_query) effective_q + 1 else hash_pos,
        query_len: if (has_query) hash_pos - (effective_q + 1) else 0,
        has_query: has_query,
        has_scheme: has_scheme,
        valid: true,
    };
}

/// Fast path-traversal guard: returns true if `pathname` contains any `..` path segment.
pub fn hasDotDotSegment(pathname: string): bool {
    let i: usize = 0;
    while (i < pathname.len) {
        // Skip delimiters '/' or '\\'
        while (i < pathname.len && (pathname[i] == 47 || pathname[i] == 92)) {
            i += 1;
        }
        const seg_start: usize = i;
        while (i < pathname.len && pathname[i] != 47 && pathname[i] != 92) {
            i += 1;
        }
        if (i - seg_start == 2 && pathname[seg_start] == 46 && pathname[seg_start + 1] == 46) {
            return true;
        }
    }
    return false;
}

/// Zero-copy slice returning the pure pathname (without scheme/authority, `?query`, or `#hash`).
/// Returns `""` if `url` is invalid or has an empty pathname.
pub fn stripQueryAndHash(url: string): string {
    const p: ParsedUrl = parseUrl(url);
    if (!p.valid || p.pathname_len == 0) {
        return "";
    }
    return url[p.pathname_start .. p.pathname_start + p.pathname_len];
}
