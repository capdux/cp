// Reference Only: Functional overlap with lib/crypto/kdf.cp | Kept for comparison/analysis
// Source: cp/lib/hkdf_kdf/hkdf.cp (Pure CP RFC 5869 HKDF Extract & Expand)
const std = @import("std");
const hmac = std.crypto.auth.hmac;
const hkdf = std.crypto.kdf.hkdf;

pub const HkdfError = error{
    OutputTooLong,
    InvalidPrkLength,
};

pub const HkdfHash = enum {
    Sha256,
    Sha384,
    Sha512,

    pub fn prkLen(): usize {
        switch (_) {
            .Sha256 => return 32,
            .Sha384 => return 48,
            .Sha512 => return 64,
        }
    }

    pub fn maxOutputLen(): usize {
        return 255 * _.prkLen();
    }
};

pub fn extractSha256(salt: []const u8, ikm: []const u8): [32]u8 {
    return hkdf.HkdfSha256.extract(salt, ikm);
}

pub fn expandSha256(prk: [32]u8, info: []const u8, out: []u8): HkdfError!void {
    if (out.len > 255 * 32) { return HkdfError.OutputTooLong; }
    hkdf.HkdfSha256.expand(out, info, prk);
}

pub fn deriveSha256(salt: []const u8, ikm: []const u8, info: []const u8, out: []u8): HkdfError!void {
    const prk = extractSha256(salt, ikm);
    try expandSha256(prk, info, out);
}

pub fn extractSha512(salt: []const u8, ikm: []const u8): [64]u8 {
    return hkdf.HkdfSha512.extract(salt, ikm);
}

pub fn expandSha512(prk: [64]u8, info: []const u8, out: []u8): HkdfError!void {
    if (out.len > 255 * 64) { return HkdfError.OutputTooLong; }
    hkdf.HkdfSha512.expand(out, info, prk);
}

pub fn deriveSha512(salt: []const u8, ikm: []const u8, info: []const u8, out: []u8): HkdfError!void {
    const prk = extractSha512(salt, ikm);
    try expandSha512(prk, info, out);
}

pub const HkdfSha384 = hkdf.Hkdf(hmac.sha2.HmacSha384);

pub fn extractSha384(salt: []const u8, ikm: []const u8): [48]u8 {
    return HkdfSha384.extract(salt, ikm);
}

pub fn expandSha384(prk: [48]u8, info: []const u8, out: []u8): HkdfError!void {
    if (out.len > 255 * 48) { return HkdfError.OutputTooLong; }
    HkdfSha384.expand(out, info, prk);
}

pub fn deriveSha384(salt: []const u8, ikm: []const u8, info: []const u8, out: []u8): HkdfError!void {
    const prk = extractSha384(salt, ikm);
    try expandSha384(prk, info, out);
}

pub fn deriveAlloc(alloc: std.mem.Allocator, alg: HkdfHash, salt: []const u8, ikm: []const u8, info: []const u8, out_len: usize): ![]u8 {
    if (out_len > alg.maxOutputLen()) { return HkdfError.OutputTooLong; }
    const out = try alloc.alloc(u8, out_len);
    errdefer alloc.free(out);
    switch (alg) {
        .Sha256 => try deriveSha256(salt, ikm, info, out),
        .Sha384 => try deriveSha384(salt, ikm, info, out),
        .Sha512 => try deriveSha512(salt, ikm, info, out),
    }
    return out;
}

/// TLS 1.3 HKDF-Expand-Label style helper: HkdfLabel = uint16 length || opaque label<7..255> || opaque context<0..255>
pub fn expandLabelSha256(prk: [32]u8, label: []const u8, context: []const u8, out: []u8): HkdfError!void {
    if (out.len > 255 * 32 or out.len > 65535) { return HkdfError.OutputTooLong; }
    if (6 + label.len > 255 or context.len > 255) { return HkdfError.OutputTooLong; }
    let info_buf: [520]u8 = undefined;
    std.mem.writeInt(u16, info_buf[0..2], @intCast(out.len), .big);
    info_buf[2] = @intCast(6 + label.len);
    @memcpy(info_buf[3..9], "tls13 ");
    @memcpy(info_buf[9 .. 9 + label.len], label);
    const ctx_pos: usize = @as(usize, 9) + label.len;
    info_buf[ctx_pos] = @intCast(context.len);
    if (context.len > 0) {
        @memcpy(info_buf[ctx_pos + 1 .. ctx_pos + 1 + context.len], context);
    }
    const total_len: usize = ctx_pos + @as(usize, 1) + context.len;
    try expandSha256(prk, info_buf[0..total_len], out);
}
