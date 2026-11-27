// @cap/querystring — Zero-allocation URL query string parser & stringifier for CP
// Clean-room MIT parity with Node.js `querystring` & npm `query-string` v9.
// Composes `@cap/percent-encoding` for RFC/WHATWG URL encoding & form decoding.

import {
    componentSet,
    percentEncode,
    percentDecodePlus,
} from "../../percent-encoding/src/index.cp";

/// A raw key-value entry extracted from a URL query string.
pub struct QueryEntry {
    pub key: string,
    pub value: string,
}

fn qstFindByte(s: string, start: usize, end: usize, target: u8): usize {
    let i = start;
    while (i < end) {
        if (s[i] == target) {
            return i;
        }
        i = i + 1;
    }
    return end;
}

fn qstStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn qstNeedsDecode(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == (0x25 as u8) || s[i] == (0x2B as u8)) { // '%' or '+'
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn qstHasSlashBefore(s: string, end: usize): bool {
    let i: usize = 0;
    while (i < end) {
        if (s[i] == (0x2F as u8)) { // '/'
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Percent-encode a query key or value into `out` via `@cap/percent-encoding`.
/// Returns total bytes written into `out`.
pub fn escapeQuery(raw: string, out: []u8): usize {
    if (raw.len == 0 || out.len == 0) {
        return 0;
    }
    return percentEncode(raw, componentSet(), out);
}

/// Percent-decode a query key or value (converting `+` to space and `%XX` to bytes)
/// into `out` via `@cap/percent-encoding`. Returns total bytes written into `out`.
pub fn unescapeQuery(raw: string, out: []u8): usize {
    if (raw.len == 0 || out.len == 0) {
        return 0;
    }
    return percentDecodePlus(raw, out);
}

/// Extract the raw query substring between `?` and `#` from a full URL or path.
/// Returns `""` if no `?` is present before `#` (matches npm `query-string.extract`).
pub fn extractUrlQuery(url: string): string {
    if (url.len == 0) {
        return "";
    }
    const hash_pos = qstFindByte(url, 0, url.len, 0x23 as u8); // '#'
    const q_pos = qstFindByte(url, 0, hash_pos, 0x3F as u8);   // '?'
    if (q_pos >= hash_pos) {
        return "";
    }
    return url[(q_pos + 1)..hash_pos];
}

/// Parse a query string into `out_entries` with zero heap allocations.
/// Automatically skips a leading `?`, truncates any `#hash` fragment, supports custom
/// `sep` (default `'&'` if `0`) and `eq` (default `'='` if `0`), and handles bare flag keys
/// (e.g. `?debug&v=1` -> `key="debug", value=""`).
/// Returns the number of entries written.
pub fn parseQueryString(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize {
    if (input.len == 0 || out_entries.len == 0) {
        return 0;
    }

    const s_sep: u8 = if (sep == 0) (0x26 as u8) else sep;
    const s_eq: u8 = if (eq == 0) (0x3D as u8) else eq;

    const end = qstFindByte(input, 0, input.len, 0x23 as u8); // truncate at '#'
    let pos: usize = 0;
    if (pos < end && input[pos] == (0x3F as u8)) { // skip leading '?'
        pos = 1;
    }

    let count: usize = 0;
    while (pos < end && count < out_entries.len) {
        const next_sep = qstFindByte(input, pos, end, s_sep);
        if (next_sep > pos) {
            const eq_pos = qstFindByte(input, pos, next_sep, s_eq);
            if (eq_pos < next_sep) {
                out_entries[count] = QueryEntry {
                    key: input[pos..eq_pos],
                    value: input[(eq_pos + 1)..next_sep],
                };
            } else {
                out_entries[count] = QueryEntry {
                    key: input[pos..next_sep],
                    value: "",
                };
            }
            count = count + 1;
        }
        if (next_sep >= end) {
            break;
        }
        pos = next_sep + 1;
    }

    return count;
}

/// Serialize `entries` into `out` using custom `sep` (default `'&'`) and `eq` (default `'='`)
/// with percent-encoding via `@cap/percent-encoding`. Returns total bytes written.
pub fn stringifyQueryString(entries: []const QueryEntry, sep: u8, eq: u8, out: []u8): usize {
    if (entries.len == 0 || out.len == 0) {
        return 0;
    }

    const s_sep: u8 = if (sep == 0) (0x26 as u8) else sep;
    const s_eq: u8 = if (eq == 0) (0x3D as u8) else eq;
    const enc_set = componentSet();

    let pos: usize = 0;
    let i: usize = 0;
    while (i < entries.len && pos < out.len) {
        if (i > 0) {
            out[pos] = s_sep;
            pos = pos + 1;
            if (pos >= out.len) {
                break;
            }
        }

        pos = pos + percentEncode(entries[i].key, enc_set, out[pos..]);
        if (pos >= out.len) {
            break;
        }

        out[pos] = s_eq;
        pos = pos + 1;
        if (pos >= out.len) {
            break;
        }

        pos = pos + percentEncode(entries[i].value, enc_set, out[pos..]);
        i = i + 1;
    }

    return pos;
}

/// Extract and percent-decode the first value matching `target_key` from either a full URL
/// (e.g. `https://example.com/search?q=hello+world#top`) or a raw query string (`?q=hello+world` / `q=hello+world`).
/// Returns decoded byte length written into `out`, or `null` if `target_key` is absent.
pub fn pickQueryParam(url_or_query: string, target_key: string, out: []u8): usize? {
    if (url_or_query.len == 0 || target_key.len == 0) {
        return null;
    }

    const end = qstFindByte(url_or_query, 0, url_or_query.len, 0x23 as u8); // '#'
    let pos: usize = 0;
    if (pos < end && url_or_query[pos] == (0x3F as u8)) {
        pos = 1;
    } else {
        const q_pos = qstFindByte(url_or_query, 0, end, 0x3F as u8);
        if (q_pos < end && qstHasSlashBefore(url_or_query, q_pos)) {
            pos = q_pos + 1;
        }
    }

    let key_buf: [256]u8 = undefined;

    while (pos < end) {
        const next_sep = qstFindByte(url_or_query, pos, end, 0x26 as u8); // '&'
        if (next_sep > pos) {
            const eq_pos = qstFindByte(url_or_query, pos, next_sep, 0x3D as u8); // '='
            const raw_key = if (eq_pos < next_sep) url_or_query[pos..eq_pos] else url_or_query[pos..next_sep];
            const raw_val = if (eq_pos < next_sep) url_or_query[(eq_pos + 1)..next_sep] else "";

            let matched = qstStrEq(raw_key, target_key);
            if (!matched && qstNeedsDecode(raw_key)) {
                const klen = percentDecodePlus(raw_key, key_buf[0..256]);
                matched = qstStrEq(key_buf[0..klen], target_key);
            }

            if (matched) {
                return percentDecodePlus(raw_val, out);
            }
        }
        if (next_sep >= end) {
            break;
        }
        pos = next_sep + 1;
    }

    return null;
}

export {
    QueryEntry,
    escapeQuery,
    unescapeQuery,
    extractUrlQuery,
    parseQueryString,
    stringifyQueryString,
    pickQueryParam,
};
