// @cap/etag — Zero-allocation RFC 9110 HTTP ETag Generator, Parser & Conditional Matcher
// Clean-room MIT implementation in pure CP.

pub struct EntityTag {
    pub weak: bool,
    pub tag: string,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn isEtagChar(b: u8) -> bool {
    // RFC 9110 etagc = %x21 / %x23-7E / obs-text (%x80-FF)
    if (b == 0x21) { return true; }
    if (b >= 0x23 && b <= 0x7E) { return true; }
    if (b >= 0x80) { return true; }
    return false;
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

/// Parses an RFC 9110 ETag (`"abc"` or `W/"abc"`) into `out`.
/// Rejects lowercase `w/`, missing quotes, and illegal control/whitespace bytes inside the tag.
pub fn parseEtag(input: string, out: *EntityTag) -> bool {
    let a: usize = 0;
    let b: usize = input.len;
    while (a < b && isSpace(input[a])) {
        a += 1;
    }
    while (b > a && isSpace(input[b - 1])) {
        b -= 1;
    }

    let weak = false;
    if (b - a >= 2 && input[a] == 87 && input[a + 1] == 47) {
        // Strict uppercase "W/"
        weak = true;
        a += 2;
    }

    if (b - a < 2 || input[a] != 34 || input[b - 1] != 34) {
        return false;
    }

    const inner_start = a + 1;
    const inner_end = b - 1;
    let i = inner_start;
    while (i < inner_end) {
        if (!isEtagChar(input[i])) {
            return false;
        }
        i += 1;
    }

    out.weak = weak;
    out.tag = input[inner_start..inner_end];
    return true;
}

/// Formats an ETag as `"<tag>"` or `W/"<tag>"` into `out`.
/// Returns bytes written, or `0` if `out` is too small.
pub fn formatEtag(weak: bool, tag: string, out: []u8) -> usize {
    let prefix_len: usize = 0;
    if (weak) {
        prefix_len = 2;
    }
    const needed = prefix_len + 1 + tag.len + 1;
    if (needed > out.len) {
        return 0;
    }
    let w: usize = 0;
    if (weak) {
        out[w] = 87;     // 'W'
        out[w + 1] = 47; // '/'
        w += 2;
    }
    out[w] = 34; // '"'
    w += 1;
    let i: usize = 0;
    while (i < tag.len) {
        out[w] = tag[i];
        w += 1;
        i += 1;
    }
    out[w] = 34; // '"'
    w += 1;
    return w;
}

/// RFC 9110 strong comparison: both tags must be strong (`!weak`) and byte-identical.
pub fn strongEqual(a: EntityTag, b: EntityTag) -> bool {
    if (a.weak || b.weak) {
        return false;
    }
    return strEq(a.tag, b.tag);
}

/// RFC 9110 weak comparison: ignores weakness flag and compares opaque-tag bytes.
pub fn weakEqual(a: EntityTag, b: EntityTag) -> bool {
    return strEq(a.tag, b.tag);
}

fn hexLower(nibble: u8) -> u8 {
    if (nibble < 10) {
        return 48 + nibble;
    }
    return 87 + nibble; // 'a' = 97 = 87 + 10
}

fn writeHexU64(val: u64, out: []u8, start: usize) -> usize {
    let rev: [16]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        rev[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            rev[n] = hexLower((v & 0x0F) as u8);
            v = v >> 4;
            n += 1;
        }
    }
    let i: usize = 0;
    while (i < n) {
        out[start + i] = rev[n - 1 - i];
        i += 1;
    }
    return n;
}

fn hexLenU64(val: u64) -> usize {
    if (val == 0) {
        return 1;
    }
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        v = v >> 4;
        n += 1;
    }
    return n;
}

/// Generates a weak stat-based ETag `W/"<hex-size>-<hex-mtime>"` (matching Express/npm `etag`).
/// Returns bytes written, or `0` if `out` is too small.
pub fn generateStatEtag(size: u64, mtime_ms: u64, out: []u8) -> usize {
    const s_len = hexLenU64(size);
    const m_len = hexLenU64(mtime_ms);
    const needed = 3 + s_len + 1 + m_len + 1; // W/" + size + - + mtime + "
    if (needed > out.len) {
        return 0;
    }
    let w: usize = 0;
    out[w] = 87;     // 'W'
    out[w + 1] = 47; // '/'
    out[w + 2] = 34; // '"'
    w += 3;
    w += writeHexU64(size, out, w);
    out[w] = 45;     // '-'
    w += 1;
    w += writeHexU64(mtime_ms, out, w);
    out[w] = 34;     // '"'
    w += 1;
    return w;
}

fn fnv1a64(data: []const u8) -> u64 {
    const offset: u64 = ((0xCBF29CE4 as u64) << 32) | (0x84222325 as u64);
    const prime: u64 = ((0x00000100 as u64) << 32) | (0x000001B3 as u64);
    let h: u64 = offset;
    let i: usize = 0;
    while (i < data.len) {
        h ^= data[i] as u64;
        h = h *% prime;
        i += 1;
    }
    return h;
}

/// Generates a deterministic content ETag `"<hex-len>-<hex-hash>"` (or `W/"..."` if `weak`)
/// using FNV-1a 64-bit. Returns bytes written, or `0` if `out` is too small.
pub fn generateContentEtag(data: []const u8, weak: bool, out: []u8) -> usize {
    const dlen = data.len as u64;
    const hash = fnv1a64(data);
    let prefix_len: usize = 0;
    if (weak) {
        prefix_len = 2;
    }
    const l_len = hexLenU64(dlen);
    const h_len = hexLenU64(hash);
    const needed = prefix_len + 1 + l_len + 1 + h_len + 1;
    if (needed > out.len) {
        return 0;
    }
    let w: usize = 0;
    if (weak) {
        out[w] = 87;
        out[w + 1] = 47;
        w += 2;
    }
    out[w] = 34; // '"'
    w += 1;
    w += writeHexU64(dlen, out, w);
    out[w] = 45; // '-'
    w += 1;
    w += writeHexU64(hash, out, w);
    out[w] = 34; // '"'
    w += 1;
    return w;
}

fn matchHeaderList(header: string, current: EntityTag, require_strong: bool) -> bool {
    let a: usize = 0;
    let b: usize = header.len;
    while (a < b && isSpace(header[a])) {
        a += 1;
    }
    while (b > a && isSpace(header[b - 1])) {
        b -= 1;
    }
    if (a >= b) {
        return false;
    }
    if (b - a == 1 && header[a] == 42) {
        return true; // '*'
    }

    let pos = a;
    while (pos < b) {
        const start = pos;
        let in_quote = false;
        while (pos < b) {
            const ch = header[pos];
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const end = pos;
        let parsed = EntityTag { weak: false, tag: "" };
        if (parseEtag(header[start..end], &parsed)) {
            if (require_strong) {
                if (strongEqual(parsed, current)) {
                    return true;
                }
            } else {
                if (weakEqual(parsed, current)) {
                    return true;
                }
            }
        }
        if (pos < b && header[pos] == 44) {
            pos += 1;
        }
    }
    return false;
}

/// Evaluates an `If-None-Match` header against `current` using RFC 9110 weak comparison.
pub fn matchIfNoneMatch(header: string, current: EntityTag) -> bool {
    return matchHeaderList(header, current, false);
}

/// Evaluates an `If-Match` header against `current` using RFC 9110 strong comparison.
pub fn matchIfMatch(header: string, current: EntityTag) -> bool {
    return matchHeaderList(header, current, true);
}
