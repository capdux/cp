// @cap/http-content-range — Zero-allocation RFC 9110 Content-Range parser & formatter in pure CP
// Clean-room MIT implementation of crates.io `http-content-range`.

pub enum ContentRangeKind {
    Invalid,
    Bytes,
    BytesUnknownLength,
    Unsatisfied,
}

pub struct ContentRange {
    pub kind: ContentRangeKind,
    pub first_byte: u64,
    pub last_byte: u64,
    pub complete_length: u64,

    pub fn isValid(): bool {
        return _.kind != .Invalid;
    }

    pub fn rangeLen(): u64 {
        if (_.kind == .Bytes || _.kind == .BytesUnknownLength) {
            if (_.last_byte >= _.first_byte) {
                return _.last_byte - _.first_byte + 1;
            }
        }
        return 0;
    }
}

fn hcrParseU64(s: string): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 48 || b > 57) {
            return null;
        }
        const digit = (b - 48) as u64;
        if (acc > 1844674407370955161) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn hcrPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn hcrPushU64(out: []u8, pos: usize, val: u64): usize {
    let rev: [20]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        rev[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            rev[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = rev[k];
        p += 1;
    }
    return p;
}

/// Parses an RFC 9110 `Content-Range` header:
/// - `bytes 0-499/1234` -> `.Bytes`
/// - `bytes 0-499/*` -> `.BytesUnknownLength`
/// - `bytes */1234` -> `.Unsatisfied`
pub fn parseContentRange(header: string): ContentRange {
    const invalid = ContentRange { kind: .Invalid, first_byte: 0, last_byte: 0, complete_length: 0 };
    let a: usize = 0;
    let b: usize = header.len;
    while (a < b && (header[a] == 32 || header[a] == 9)) {
        a += 1;
    }
    while (b > a && (header[b - 1] == 32 || header[b - 1] == 9)) {
        b -= 1;
    }
    const s = header[a..b];
    // Must start with "bytes "
    if (s.len < 8) {
        return invalid;
    }
    if (s[0] != 98 || s[1] != 121 || s[2] != 116 || s[3] != 101 || s[4] != 115 || s[5] != 32) {
        return invalid;
    }
    const rest = s[6..s.len];
    // Find '/'
    let slash: usize = rest.len;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == 47) {
            slash = i;
            break;
        }
        i += 1;
    }
    if (slash == 0 || slash + 1 >= rest.len) {
        return invalid;
    }
    const range_part = rest[0..slash];
    const size_part = rest[(slash + 1)..rest.len];

    // Unsatisfied form: "bytes */<size>"
    if (range_part.len == 1 && range_part[0] == 42) {
        if (size_part.len == 1 && size_part[0] == 42) {
            return invalid; // "bytes */*" is illegal per RFC 9110
        }
        if (sz := hcrParseU64(size_part)) {
            return ContentRange {
                kind: .Unsatisfied,
                first_byte: 0,
                last_byte: 0,
                complete_length: sz,
            };
        }
        return invalid;
    }

    // Range form: "<first>-<last>/<size|*>"
    let dash: usize = range_part.len;
    let d: usize = 0;
    while (d < range_part.len) {
        if (range_part[d] == 45) {
            dash = d;
            break;
        }
        d += 1;
    }
    if (dash == 0 || dash + 1 >= range_part.len) {
        return invalid;
    }
    const first_opt = hcrParseU64(range_part[0..dash]);
    const last_opt = hcrParseU64(range_part[(dash + 1)..range_part.len]);
    if (first_opt == null || last_opt == null) {
        return invalid;
    }
    const first = first_opt ?? 0;
    const last = last_opt ?? 0;
    if (first > last) {
        return invalid;
    }

    if (size_part.len == 1 && size_part[0] == 42) {
        return ContentRange {
            kind: .BytesUnknownLength,
            first_byte: first,
            last_byte: last,
            complete_length: 0,
        };
    }
    if (sz := hcrParseU64(size_part)) {
        if (last >= sz) {
            return invalid; // RFC 9110: last_byte must be < complete_length
        }
        return ContentRange {
            kind: .Bytes,
            first_byte: first,
            last_byte: last,
            complete_length: sz,
        };
    }
    return invalid;
}

/// Formats a `ContentRange` back into an RFC 9110 header string. Returns `0` for `.Invalid`.
pub fn formatContentRangeHeader(cr: ContentRange, out: []u8): usize {
    if (cr.kind == .Invalid) {
        return 0;
    }
    let p = hcrPush(out, 0, "bytes ");
    if (cr.kind == .Unsatisfied) {
        p = hcrPush(out, p, "*/");
        return hcrPushU64(out, p, cr.complete_length);
    }
    p = hcrPushU64(out, p, cr.first_byte);
    p = hcrPush(out, p, "-");
    p = hcrPushU64(out, p, cr.last_byte);
    p = hcrPush(out, p, "/");
    if (cr.kind == .BytesUnknownLength) {
        return hcrPush(out, p, "*");
    }
    return hcrPushU64(out, p, cr.complete_length);
}

export {
    ContentRangeKind,
    ContentRange,
    parseContentRange,
    formatContentRangeHeader,
};
