// @cap/qs-lite — Zero-heap application/x-www-form-urlencoded parser & serializer
// Clean-room MIT implementation for CP (compatible with npm qs, query-string & Rust serde_urlencoded).

pub struct QueryPair {
    pub key: string,
    pub value: string,
}

fn hexVal(c: u8): i32 {
    if (c >= '0' && c <= '9') { return (c - '0') as i32; }
    if (c >= 'a' && c <= 'f') { return (c - 'a' + 10) as i32; }
    if (c >= 'A' && c <= 'F') { return (c - 'A' + 10) as i32; }
    return -1;
}

fn hexDigit(n: u8): u8 {
    if (n < 10) { return ('0' as u8) + n; }
    return ('A' as u8) + (n - 10);
}

fn isUnreserved(c: u8): bool {
    if (c >= 'A' && c <= 'Z') { return true; }
    if (c >= 'a' && c <= 'z') { return true; }
    if (c >= '0' && c <= '9') { return true; }
    return c == '-' || c == '.' || c == '_' || c == '~';
}

/// Percent-encode a form component (`+` for space when `space_as_plus` is true, else `%20`).
/// Returns bytes written to `out` (0 on insufficient buffer for non-empty input).
pub fn encodeComponent(input: string, space_as_plus: bool, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let c: u8 = input[i];
        if (isUnreserved(c)) {
            if (pos >= out.len) { return 0; }
            out[pos] = c;
            pos += 1;
        } else if (c == ' ' && space_as_plus) {
            if (pos >= out.len) { return 0; }
            out[pos] = '+';
            pos += 1;
        } else {
            if (pos + 3 > out.len) { return 0; }
            out[pos] = '%';
            out[pos + 1] = hexDigit((c >> 4) & 0x0f);
            out[pos + 2] = hexDigit(c & 0x0f);
            pos += 3;
        }
        i += 1;
    }
    return pos;
}

/// Percent-decode a form component (`+` decoded to `' '` when `plus_as_space` is true).
/// Returns bytes written to `out`.
pub fn decodeComponent(input: string, plus_as_space: bool, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (pos >= out.len) { return 0; }
        let c: u8 = input[i];
        if (c == '+' && plus_as_space) {
            out[pos] = ' ';
            pos += 1;
            i += 1;
        } else if (c == '%' && i + 2 < input.len) {
            let hi: i32 = hexVal(input[i + 1]);
            let lo: i32 = hexVal(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[pos] = (((hi as u8) << 4) | (lo as u8));
                pos += 1;
                i += 3;
            } else {
                out[pos] = c;
                pos += 1;
                i += 1;
            }
        } else {
            out[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

/// Zero-copy split of a query string (automatically stripping a leading `'?'`) into raw `QueryPair` slices.
/// Returns number of pairs written to `out`.
pub fn parseRaw(query: string, out: []QueryPair): usize {
    let start: usize = 0;
    if (query.len > 0 && query[0] == '?') {
        start = 1;
    }
    let count: usize = 0;
    let i: usize = start;
    while (i <= query.len && count < out.len) {
        let seg_start: usize = i;
        while (i < query.len && query[i] != '&') {
            i += 1;
        }
        let seg_end: usize = i;
        if (seg_end > seg_start) {
            let eq: usize = seg_start;
            while (eq < seg_end && query[eq] != '=') {
                eq += 1;
            }
            if (eq < seg_end) {
                out[count] = QueryPair {
                    key: query[seg_start..eq],
                    value: query[(eq + 1)..seg_end],
                };
            } else {
                out[count] = QueryPair {
                    key: query[seg_start..seg_end],
                    value: "",
                };
            }
            count += 1;
        }
        if (i >= query.len) {
            break;
        }
        i += 1;
    }
    return count;
}

/// Count how many non-empty key-value pairs exist in a query string.
pub fn countPairs(query: string): usize {
    let start: usize = 0;
    if (query.len > 0 && query[0] == '?') {
        start = 1;
    }
    let count: usize = 0;
    let i: usize = start;
    while (i < query.len) {
        let seg_start: usize = i;
        while (i < query.len && query[i] != '&') {
            i += 1;
        }
        if (i > seg_start) {
            count += 1;
        }
        if (i < query.len) {
            i += 1;
        }
    }
    return count;
}

fn eqBytes(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

/// Look up the first decoded value for `target_key` in `query` and write it into `out`.
/// Returns `true` if found (writing decoded length into `out_len`), `false` otherwise.
pub fn getDecoded(query: string, target_key: string, out: []u8, out_len: *usize): bool {
    let start: usize = 0;
    if (query.len > 0 && query[0] == '?') {
        start = 1;
    }
    let key_buf: [256]u8 = undefined;
    let i: usize = start;
    while (i < query.len) {
        let seg_start: usize = i;
        while (i < query.len && query[i] != '&') {
            i += 1;
        }
        let seg_end: usize = i;
        if (seg_end > seg_start) {
            let eq: usize = seg_start;
            while (eq < seg_end && query[eq] != '=') {
                eq += 1;
            }
            let raw_k: string = query[seg_start..eq];
            let klen: usize = decodeComponent(raw_k, true, key_buf[0..]);
            if (eqBytes(key_buf[0..klen], target_key)) {
                if (eq < seg_end) {
                    let raw_v: string = query[(eq + 1)..seg_end];
                    if (raw_v.len == 0) {
                        out_len.* = 0;
                    } else {
                        out_len.* = decodeComponent(raw_v, true, out);
                    }
                } else {
                    out_len.* = 0;
                }
                return true;
            }
        }
        if (i < query.len) {
            i += 1;
        }
    }
    out_len.* = 0;
    return false;
}

/// Serialize `pairs` into `application/x-www-form-urlencoded` format (`k1=v1&k2=v2`).
/// Returns bytes written to `out` (0 on empty input or insufficient buffer).
pub fn stringify(pairs: []const QueryPair, space_as_plus: bool, out: []u8): usize {
    if (pairs.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < pairs.len) {
        if (i > 0) {
            if (pos >= out.len) { return 0; }
            out[pos] = '&';
            pos += 1;
        }
        if (pairs[i].key.len > 0) {
            let kn: usize = encodeComponent(pairs[i].key, space_as_plus, out[pos..]);
            if (kn == 0) { return 0; }
            pos += kn;
        }
        if (pos >= out.len) { return 0; }
        out[pos] = '=';
        pos += 1;
        if (pairs[i].value.len > 0) {
            let vn: usize = encodeComponent(pairs[i].value, space_as_plus, out[pos..]);
            if (vn == 0) { return 0; }
            pos += vn;
        }
        i += 1;
    }
    return pos;
}
