// Origin: Pure-CP Stdlib (cap:url/fastDecode 16-Lane SIMD URI Decoder & Query Helpers) | Created: 2026-11-25
// ============================================================================
// Pure-CP zero-heap, 16-lane SIMD accelerated URI component decoder (`cap:url/fastDecode`):
// - SIMD fastDecodeUriComponent (16-lane @Vector(16, u8) scan for '%' & '+')
// - Zero-heap query string pair parser & decoded parameter lookup
// ============================================================================

type UrlFastVec16u8 = @Vector(16, u8);

fn urlHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD URI component decoder with optional `+` -> `' '` conversion.
/// Fast-paths 16-byte chunks containing no `%` (or `+` when `plus_as_space` is true).
/// Returns decoded byte count, or `null` on invalid/truncated `%XX` escape or insufficient `out_buf`.
pub fn fastDecodeUriComponentOpts(input: string, out_buf: []u8, plus_as_space: bool): usize? {
    const n: usize = input.len;
    if (n == 0) return 0;

    let i: usize = 0;
    let out_pos: usize = 0;
    const v_pct: UrlFastVec16u8 = @splat('%');
    const v_plus: UrlFastVec16u8 = @splat('+');

    while (i + 16 <= n) {
        const chunk: UrlFastVec16u8 = input[i..][0..16].*;
        const pct_mask: u16 = @bitCast(chunk == v_pct);
        const mask: u16 = if (plus_as_space) (pct_mask | @as(u16, @bitCast(chunk == v_plus))) else pct_mask;

        if (mask == 0) {
            if (out_pos + 16 > out_buf.len) return null;
            out_buf[out_pos..][0..16].* = chunk;
            out_pos += 16;
            i += 16;
            continue;
        }

        const run: usize = @as(usize, @ctz(mask));
        if (run > 0) {
            if (out_pos + run > out_buf.len) return null;
            let k: usize = 0;
            while (k < run) : (k += 1) {
                out_buf[out_pos + k] = input[i + k];
            }
            out_pos += run;
            i += run;
        }

        const ch = input[i];
        if (ch == '+') {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = ' ';
            out_pos += 1;
            i += 1;
        } else {
            if (i + 2 >= n) return null;
            const hi = urlHexNibble(input[i + 1]) orelse return null;
            const lo = urlHexNibble(input[i + 2]) orelse return null;
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = (hi << 4) | lo;
            out_pos += 1;
            i += 3;
        }
    }

    while (i < n) {
        const ch = input[i];
        if (ch == '%') {
            if (i + 2 >= n) return null;
            const hi = urlHexNibble(input[i + 1]) orelse return null;
            const lo = urlHexNibble(input[i + 2]) orelse return null;
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = (hi << 4) | lo;
            out_pos += 1;
            i += 3;
        } else if (plus_as_space && ch == '+') {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = ' ';
            out_pos += 1;
            i += 1;
        } else {
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = ch;
            out_pos += 1;
            i += 1;
        }
    }

    return out_pos;
}

/// 16-lane SIMD `decodeURIComponent` equivalent (`+` kept literal, strict `%XX` hex validation).
pub fn fastDecodeUriComponent(input: string, out_buf: []u8): usize? {
    return fastDecodeUriComponentOpts(input, out_buf, false);
}

/// 16-lane SIMD `application/x-www-form-urlencoded` decoder (`+` decoded as space).
pub fn fastDecodeFormComponent(input: string, out_buf: []u8): usize? {
    return fastDecodeUriComponentOpts(input, out_buf, true);
}

/// Zero-copy raw query string key-value pair.
pub struct QueryPair {
    key: string = "",
    value: string = "",
}

/// Splits a single `key=value` query segment on the first `=`.
pub fn parseQueryPair(segment: string): QueryPair {
    let i: usize = 0;
    while (i < segment.len) : (i += 1) {
        if (segment[i] == '=') {
            return QueryPair{
                key: segment[0..i],
                value: segment[i + 1 ..],
            };
        }
    }
    return QueryPair{
        key: segment,
        value: "",
    };
}

/// Finds `target_key` in a query string (with or without leading `?`) and decodes its value into `out_buf`.
/// Supports `+` as space and `%XX` decoding on both key and value.
pub fn findQueryParamDecoded(query: string, target_key: string, out_buf: []u8): usize? {
    if (query.len == 0) return null;
    let pos: usize = if (query[0] == '?') 1 else 0;
    let key_buf: [256]u8 = undefined;

    while (pos <= query.len) {
        let end: usize = pos;
        while (end < query.len && query[end] != '&') : (end += 1) {}
        if (end > pos) {
            const pair = parseQueryPair(query[pos..end]);
            let key_match = false;
            if (std.mem.eql(u8, pair.key, target_key)) {
                key_match = true;
            } else if (pair.key.len <= key_buf.len) {
                if (klen := fastDecodeUriComponentOpts(pair.key, key_buf[0..], true)) {
                    if (std.mem.eql(u8, key_buf[0..klen], target_key)) {
                        key_match = true;
                    }
                }
            }
            if (key_match) {
                return fastDecodeUriComponentOpts(pair.value, out_buf, true);
            }
        }
        if (end >= query.len) break;
        pos = end + 1;
    }
    return null;
}

