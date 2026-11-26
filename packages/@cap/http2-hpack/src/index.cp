// packages/@cap/http2-hpack/src/index.cp
// Zero-allocation RFC 7541 HTTP/2 HPACK static table & N-bit prefixed integer codec.
// MIT clean-room pure CP implementation.

/// An entry in the RFC 7541 Appendix A 61-entry HPACK static table.
pub struct StaticEntry {
    pub name: string,
    pub value: string,
    pub valid: bool,
}

/// Result of decoding an RFC 7541 Section 5.1 N-bit prefixed integer.
pub struct IntDecodeResult {
    pub value: u64,
    pub consumed: usize,
    pub valid: bool,
}

fn hpackEqBytes(a: []const u8, b: []const u8): bool {
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

fn hpackEntry(n: string, v: string): StaticEntry {
    return StaticEntry{
        name: n,
        value: v,
        valid: true,
    };
}

/// Look up an entry in the RFC 7541 Appendix A HPACK static table (1-based index `1..61`).
/// Returns `valid: false` if `index` is out of range.
pub fn staticTableGet(index: usize): StaticEntry {
    if (index == 1) { return hpackEntry(":authority", ""); }
    if (index == 2) { return hpackEntry(":method", "GET"); }
    if (index == 3) { return hpackEntry(":method", "POST"); }
    if (index == 4) { return hpackEntry(":path", "/"); }
    if (index == 5) { return hpackEntry(":path", "/index.html"); }
    if (index == 6) { return hpackEntry(":scheme", "http"); }
    if (index == 7) { return hpackEntry(":scheme", "https"); }
    if (index == 8) { return hpackEntry(":status", "200"); }
    if (index == 9) { return hpackEntry(":status", "204"); }
    if (index == 10) { return hpackEntry(":status", "206"); }
    if (index == 11) { return hpackEntry(":status", "304"); }
    if (index == 12) { return hpackEntry(":status", "400"); }
    if (index == 13) { return hpackEntry(":status", "404"); }
    if (index == 14) { return hpackEntry(":status", "500"); }
    if (index == 15) { return hpackEntry("accept-charset", ""); }
    if (index == 16) { return hpackEntry("accept-encoding", "gzip, deflate"); }
    if (index == 17) { return hpackEntry("accept-language", ""); }
    if (index == 18) { return hpackEntry("accept-ranges", ""); }
    if (index == 19) { return hpackEntry("accept", ""); }
    if (index == 20) { return hpackEntry("access-control-allow-origin", ""); }
    if (index == 21) { return hpackEntry("age", ""); }
    if (index == 22) { return hpackEntry("allow", ""); }
    if (index == 23) { return hpackEntry("authorization", ""); }
    if (index == 24) { return hpackEntry("cache-control", ""); }
    if (index == 25) { return hpackEntry("content-disposition", ""); }
    if (index == 26) { return hpackEntry("content-encoding", ""); }
    if (index == 27) { return hpackEntry("content-language", ""); }
    if (index == 28) { return hpackEntry("content-length", ""); }
    if (index == 29) { return hpackEntry("content-location", ""); }
    if (index == 30) { return hpackEntry("content-range", ""); }
    if (index == 31) { return hpackEntry("content-type", ""); }
    if (index == 32) { return hpackEntry("cookie", ""); }
    if (index == 33) { return hpackEntry("date", ""); }
    if (index == 34) { return hpackEntry("etag", ""); }
    if (index == 35) { return hpackEntry("expect", ""); }
    if (index == 36) { return hpackEntry("expires", ""); }
    if (index == 37) { return hpackEntry("from", ""); }
    if (index == 38) { return hpackEntry("host", ""); }
    if (index == 39) { return hpackEntry("if-match", ""); }
    if (index == 40) { return hpackEntry("if-modified-since", ""); }
    if (index == 41) { return hpackEntry("if-none-match", ""); }
    if (index == 42) { return hpackEntry("if-range", ""); }
    if (index == 43) { return hpackEntry("if-unmodified-since", ""); }
    if (index == 44) { return hpackEntry("last-modified", ""); }
    if (index == 45) { return hpackEntry("link", ""); }
    if (index == 46) { return hpackEntry("location", ""); }
    if (index == 47) { return hpackEntry("max-forwards", ""); }
    if (index == 48) { return hpackEntry("proxy-authenticate", ""); }
    if (index == 49) { return hpackEntry("proxy-authorization", ""); }
    if (index == 50) { return hpackEntry("range", ""); }
    if (index == 51) { return hpackEntry("referer", ""); }
    if (index == 52) { return hpackEntry("refresh", ""); }
    if (index == 53) { return hpackEntry("retry-after", ""); }
    if (index == 54) { return hpackEntry("server", ""); }
    if (index == 55) { return hpackEntry("set-cookie", ""); }
    if (index == 56) { return hpackEntry("strict-transport-security", ""); }
    if (index == 57) { return hpackEntry("transfer-encoding", ""); }
    if (index == 58) { return hpackEntry("user-agent", ""); }
    if (index == 59) { return hpackEntry("vary", ""); }
    if (index == 60) { return hpackEntry("via", ""); }
    if (index == 61) { return hpackEntry("www-authenticate", ""); }
    return StaticEntry{ name: "", value: "", valid: false };
}

/// Find an exact `(name, value)` match in the HPACK static table (returns `1..61`, or `0` if not found).
pub fn staticTableFind(name: string, value: string): usize {
    let i: usize = 1;
    while (i <= 61) {
        const entry = staticTableGet(i);
        if (hpackEqBytes(entry.name, name) && hpackEqBytes(entry.value, value)) {
            return i;
        }
        i += 1;
    }
    return 0;
}

/// Find the first static table entry matching `name` (returns `1..61`, or `0` if not found).
pub fn staticTableFindName(name: string): usize {
    let i: usize = 1;
    while (i <= 61) {
        const entry = staticTableGet(i);
        if (hpackEqBytes(entry.name, name)) {
            return i;
        }
        i += 1;
    }
    return 0;
}

/// Encode an unsigned integer using RFC 7541 Section 5.1 N-bit prefix representation (`prefix_bits` in `1..8`).
/// High bits above the prefix are taken from `first_byte_mask`.
/// Returns bytes written into `out`, or `0` on invalid `prefix_bits` or insufficient `out` buffer.
pub fn encodePrefixedInt(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize {
    if (prefix_bits < (1 as u8) || prefix_bits > (8 as u8) || out.len == 0) {
        return 0;
    }
    const maxPrefixU16: u16 = ((1 as u16) << (prefix_bits as u4)) - (1 as u16);
    const maxPrefix: u64 = maxPrefixU16 as u64;
    const clearMask: u8 = (0xFF as u8) - (maxPrefixU16 as u8);

    if (val < maxPrefix) {
        out[0] = (first_byte_mask & clearMask) | (val as u8);
        return 1;
    }

    out[0] = (first_byte_mask & clearMask) | (maxPrefixU16 as u8);
    let rem = val - maxPrefix;
    let pos: usize = 1;

    while (rem >= (128 as u64)) {
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = ((rem & (0x7F as u64)) as u8) | (0x80 as u8);
        rem = rem >> (7 as u6);
        pos += 1;
    }

    if (pos >= out.len) {
        return 0;
    }
    out[pos] = rem as u8;
    pos += 1;
    return pos;
}

/// Decode an RFC 7541 Section 5.1 N-bit prefixed integer (`prefix_bits` in `1..8`).
/// Protects against truncated inputs and 64-bit integer overflow.
pub fn decodePrefixedInt(data: []const u8, prefix_bits: u8): IntDecodeResult {
    let invalid = IntDecodeResult{
        value: 0 as u64,
        consumed: 0,
        valid: false,
    };
    if (prefix_bits < (1 as u8) || prefix_bits > (8 as u8) || data.len == 0) {
        return invalid;
    }

    const maxPrefixU16: u16 = ((1 as u16) << (prefix_bits as u4)) - (1 as u16);
    const maxPrefix: u64 = maxPrefixU16 as u64;
    let val: u64 = (data[0] as u64) & maxPrefix;
    if (val < maxPrefix) {
        return IntDecodeResult{
            value: val,
            consumed: 1,
            valid: true,
        };
    }

    const maxU64: u64 = ((0xFFFFFFFF as u64) << (32 as u6)) | (0xFFFFFFFF as u64);
    let pos: usize = 1;
    let shift: u32 = 0 as u32;

    while (pos < data.len) {
        const b = data[pos];
        pos += 1;
        const low7: u64 = (b & (0x7F as u8)) as u64;

        if (shift >= (64 as u32)) {
            return invalid;
        }
        if (shift == (63 as u32) && low7 > (1 as u64)) {
            return invalid;
        }
        const addend = low7 << (shift as u6);
        if (addend > (maxU64 - val)) {
            return invalid;
        }
        val += addend;

        if ((b & (0x80 as u8)) == (0 as u8)) {
            return IntDecodeResult{
                value: val,
                consumed: pos,
                valid: true,
            };
        }
        shift += 7 as u32;
    }

    // Input truncated before continuation bit cleared.
    return invalid;
}

/// Encode an RFC 7541 Section 6.1 Indexed Header Field (`1xxxxxxx`, 7-bit prefix).
/// Index `0` is invalid per RFC 7541 and returns `0`.
pub fn encodeIndexedHeader(index: usize, out: []u8): usize {
    if (index == 0) {
        return 0;
    }
    return encodePrefixedInt(index as u64, 7 as u8, 0x80 as u8, out);
}

export {
    StaticEntry,
    IntDecodeResult,
    staticTableGet,
    staticTableFind,
    staticTableFindName,
    encodePrefixedInt,
    decodePrefixedInt,
    encodeIndexedHeader,
};
