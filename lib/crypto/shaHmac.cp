// Reference-hardened against RustCrypto/hashes (MIT/Apache-2.0)
// Source: bun/src/sha_hmac/hmac.cp -> cp/lib/sha_hmac/hmac.cp (Pure CP unified HMAC & constant-time verification)

const std = @import("std");
import { HashAlg, ShaError, MAX_DIGEST_LEN, digestLen, digestHex, ctEqualSimd, secureZeroBytes } from "./shaHash.cp";

const HmacBlake2b256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2b256);
const HmacBlake2s256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2s256);

const HmacState = union(enum) {
    Sha1: std.crypto.auth.hmac.HmacSha1,
    Sha224: std.crypto.auth.hmac.sha2.HmacSha224,
    Sha256: std.crypto.auth.hmac.sha2.HmacSha256,
    Sha384: std.crypto.auth.hmac.sha2.HmacSha384,
    Sha512: std.crypto.auth.hmac.sha2.HmacSha512,
    Blake2b256: HmacBlake2b256,
    Blake2s256: HmacBlake2s256,
};

pub struct StreamingHmac {
    alg: HashAlg,
    state: HmacState,

    pub fn init(alg: HashAlg, key: []const u8): StreamingHmac {
        const st: HmacState = switch (alg) {
            .Sha1 => HmacState{ .Sha1 = std.crypto.auth.hmac.HmacSha1.init(key) },
            .Sha224 => HmacState{ .Sha224 = std.crypto.auth.hmac.sha2.HmacSha224.init(key) },
            .Sha256 => HmacState{ .Sha256 = std.crypto.auth.hmac.sha2.HmacSha256.init(key) },
            .Sha384 => HmacState{ .Sha384 = std.crypto.auth.hmac.sha2.HmacSha384.init(key) },
            .Sha512 => HmacState{ .Sha512 = std.crypto.auth.hmac.sha2.HmacSha512.init(key) },
            .Blake2b256 => HmacState{ .Blake2b256 = HmacBlake2b256.init(key) },
            .Blake2s256 => HmacState{ .Blake2s256 = HmacBlake2s256.init(key) },
        };
        return StreamingHmac{
            .alg = alg,
            .state = st,
        };
    }

    pub fn update(*_, data: []const u8) {
        switch (_.state) {
            .Sha1 => |*h| h.update(data),
            .Sha224 => |*h| h.update(data),
            .Sha256 => |*h| h.update(data),
            .Sha384 => |*h| h.update(data),
            .Sha512 => |*h| h.update(data),
            .Blake2b256 => |*h| h.update(data),
            .Blake2s256 => |*h| h.update(data),
        }
    }

    pub fn final(*_, out_buf: []u8): ![]const u8 {
        const dlen = digestLen(_.alg);
        if (out_buf.len < dlen) {
            return ShaError.BufferTooSmall;
        }
        switch (_.state) {
            .Sha1 => |*h| h.final(out_buf[0..20]),
            .Sha224 => |*h| h.final(out_buf[0..28]),
            .Sha256 => |*h| h.final(out_buf[0..32]),
            .Sha384 => |*h| h.final(out_buf[0..48]),
            .Sha512 => |*h| h.final(out_buf[0..64]),
            .Blake2b256 => |*h| h.final(out_buf[0..32]),
            .Blake2s256 => |*h| h.final(out_buf[0..32]),
        }
        return out_buf[0..dlen];
    }

    pub fn finalHex(*_, alloc: std.mem.Allocator): ![]u8 {
        let raw_buf: [MAX_DIGEST_LEN]u8 = undefined;
        const tag = try _.final(&raw_buf);
        return try digestHex(alloc, tag);
    }
}

pub fn hmacOneshot(alloc: std.mem.Allocator, alg: HashAlg, key: []const u8, msg: []const u8): ![]u8 {
    const dlen = digestLen(alg);
    let out = try alloc.alloc(u8, dlen);
    errdefer alloc.free(out);
    let mac = StreamingHmac.init(alg, key);
    mac.update(msg);
    * = try mac.final(out);
    return out;
}

pub fn hmacHex(alloc: std.mem.Allocator, alg: HashAlg, key: []const u8, msg: []const u8): ![]u8 {
    let raw_buf: [MAX_DIGEST_LEN]u8 = undefined;
    let mac = StreamingHmac.init(alg, key);
    mac.update(msg);
    const tag = try mac.final(&raw_buf);
    return try digestHex(alloc, tag);
}

pub fn verifyHmac(alg: HashAlg, key: []const u8, msg: []const u8, expected_tag: []const u8): bool {
    const dlen = digestLen(alg);
    if (expected_tag.len != dlen) {
        return false;
    }

    let actual_padded: [MAX_DIGEST_LEN]u8 = undefined;
    let expected_padded: [MAX_DIGEST_LEN]u8 = undefined;
    @memset(&actual_padded, 0);
    @memset(&expected_padded, 0);

    let mac = StreamingHmac.init(alg, key);
    mac.update(msg);
    * = mac.final(&actual_padded) catch return false;

    @memcpy(expected_padded[0..dlen], expected_tag);
    const ok = ctEqualSimd(&actual_padded, &expected_padded);
    secureZeroBytes(&actual_padded);
    secureZeroBytes(&expected_padded);
    return ok;
}

/// Zero-heap oneshot HMAC into caller-provided `out_buf`.
pub fn hmacInto(alg: HashAlg, key: []const u8, msg: []const u8, out_buf: []u8): ![]const u8 {
    let mac = StreamingHmac.init(alg, key);
    mac.update(msg);
    return try mac.final(out_buf);
}

/// 16-lane `@Vector(16, u8)` XOR for HMAC `ipad=0x36` / `opad=0x5C` key block padding.
pub fn xorKeyPadSimd(dst: []u8, key_block: []const u8, pad_byte: u8) {
    const n = if (dst.len < key_block.len) dst.len else key_block.len;
    const pad_v: @Vector(16, u8) = @splat(pad_byte);
    let i: usize = 0;
    while (i + 16 <= n) : (i += 16) {
        const kv: @Vector(16, u8) = key_block[i..][0..16].*;
        dst[i..][0..16].* = kv ^ pad_v;
    }
    while (i < n) : (i += 1) {
        dst[i] = key_block[i] ^ pad_byte;
    }
}

/// Scalar reference key-pad XOR for 0-diff parity testing against `xorKeyPadSimd`.
pub fn xorKeyPadScalar(dst: []u8, key_block: []const u8, pad_byte: u8) {
    const n = if (dst.len < key_block.len) dst.len else key_block.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = key_block[i] ^ pad_byte;
    }
}
