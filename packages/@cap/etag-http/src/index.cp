// @cap/etag-http — Zero-heap RFC 7232 HTTP ETag generator, parser, and comparator.
// Clean-room MIT port of npm `etag` v1.8; reuses `@cap/fnv-hash` for fast 64-bit entity hashing.

import { fnv1a64, formatHex64 } from "../../fnv-hash/src/index.cp";

pub struct ParsedEtag {
    pub valid: bool,
    pub weak: bool,
    pub opaque_tag: string,
}

fn etgPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn etgPushHexU64(out: []u8, pos: usize, v: u64): usize {
    let hex: string = "0123456789abcdef";
    let tmp: [16]u8 = undefined;
    let len: usize = 0;
    if (v == 0) {
        tmp[0] = 48;
        len = 1;
    } else {
        let cur = v;
        while (cur > 0 && len < 16) {
            tmp[len] = hex[(cur & 15) as usize];
            cur = cur >> 4;
            len = len + 1;
        }
    }
    let p = pos;
    let k = len;
    while (k > 0 && p < out.len) {
        k = k - 1;
        out[p] = tmp[k];
        p = p + 1;
    }
    return p;
}

fn etgStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

/// Generates an RFC 7232 entity tag (`"<hex_len>-<16_hex_hash>"` or `W/"..."`) for `entity` into `out`.
pub fn generateEtag(entity: []const u8, weak: bool, out: []u8): usize {
    let p: usize = 0;
    if (weak) {
        p = etgPush(out, p, "W/");
    }
    p = etgPush(out, p, "\"");
    p = etgPushHexU64(out, p, entity.len as u64);
    p = etgPush(out, p, "-");
    let h = fnv1a64(entity);
    let hbuf: [16]u8 = undefined;
    let hn = formatHex64(h, hbuf[0..16]);
    let i: usize = 0;
    while (i < hn && p < out.len) {
        out[p] = hbuf[i];
        p = p + 1;
        i = i + 1;
    }
    p = etgPush(out, p, "\"");
    return p;
}

/// Generates a weak stat-based ETag (`W/"<hex_size>-<hex_mtime>"`) from file metadata (like npm `etag(stat)`).
pub fn generateStatEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize {
    let p: usize = 0;
    p = etgPush(out, p, "W/\"");
    p = etgPushHexU64(out, p, size_bytes);
    p = etgPush(out, p, "-");
    p = etgPushHexU64(out, p, mtime_ms);
    p = etgPush(out, p, "\"");
    return p;
}

/// Parses an RFC 7232 ETag (`"abc"` or `W/"abc"`), validating etagc character constraints.
pub fn parseEtag(raw: string): ParsedEtag {
    let invalid = ParsedEtag { valid: false, weak: false, opaque_tag: "" };
    let weak = false;
    let s = raw;
    if (s.len >= 2 && s[0] == 87 && s[1] == 47) { // "W/"
        weak = true;
        s = s[2..s.len];
    }
    if (s.len < 2 || s[0] != 34 || s[s.len - 1] != 34) {
        return invalid;
    }
    let inner = s[1..s.len - 1];
    let i: usize = 0;
    while (i < inner.len) {
        let b = inner[i];
        // RFC 7232 etagc: %x21 / %x23-7E / obs-text (%x80-FF); rejects '"' (0x22), SP (0x20), CTL (<0x20, 0x7F)
        if (b < 0x21 || b == 0x22 || b == 0x7F) {
            return invalid;
        }
        i = i + 1;
    }
    return ParsedEtag { valid: true, weak: weak, opaque_tag: inner };
}

/// RFC 7232 strong comparison: both must be valid, strong (not weak), and have identical opaque-tag.
pub fn compareStrong(a: string, b: string): bool {
    let pa = parseEtag(a);
    let pb = parseEtag(b);
    if (!pa.valid || !pb.valid || pa.weak || pb.weak) { return false; }
    return etgStrEq(pa.opaque_tag, pb.opaque_tag);
}

/// RFC 7232 weak comparison: both must be valid and have identical opaque-tag (ignoring `W/`).
pub fn compareWeak(a: string, b: string): bool {
    let pa = parseEtag(a);
    let pb = parseEtag(b);
    if (!pa.valid || !pb.valid) { return false; }
    return etgStrEq(pa.opaque_tag, pb.opaque_tag);
}
