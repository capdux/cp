// Reference-hardened against RustCrypto/hashes (MIT/Apache-2.0)
// Source: cp/lib/sha_hmac/kdf.cp (Pure CP RFC 5869 HKDF & RFC 2898 PBKDF2-HMAC)

const std = @import("std");
import { HashAlg, ShaError, MAX_DIGEST_LEN, digestLen, secureZeroBytes } from "./shaHash.cp";
import { StreamingHmac } from "./shaHmac.cp";

const KdfHmacBlake2b256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2b256);
const KdfHmacBlake2s256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2s256);

pub fn hkdfExtract(alloc: std.mem.Allocator, alg: HashAlg, salt: []const u8, ikm: []const u8): ![]u8 {
    const dlen = digestLen(alg);
    let prk = try alloc.alloc(u8, dlen);
    errdefer alloc.free(prk);

    let mac = StreamingHmac.init(alg, salt);
    mac.update(ikm);
    * = try mac.final(prk);
    return prk;
}

pub fn hkdfExpand(alloc: std.mem.Allocator, alg: HashAlg, prk: []const u8, info: []const u8, out_len: usize): ![]u8 {
    const dlen = digestLen(alg);
    if (prk.len < dlen) {
        return ShaError.InvalidLength;
    }
    if (out_len > dlen * 255) {
        return ShaError.OutputTooLong;
    }

    let okm = try alloc.alloc(u8, out_len);
    errdefer alloc.free(okm);
    if (out_len == 0) {
        return okm;
    }

    switch (alg) {
        .Sha256 => {
            let prk_arr: [32]u8 = undefined;
            @memcpy(&prk_arr, prk[0..32]);
            std.crypto.kdf.hkdf.HkdfSha256.expand(okm, info, prk_arr);
            return okm;
        },
        .Sha512 => {
            let prk_arr: [64]u8 = undefined;
            @memcpy(&prk_arr, prk[0..64]);
            std.crypto.kdf.hkdf.HkdfSha512.expand(okm, info, prk_arr);
            return okm;
        },
        .Sha1 => {
            const HkdfSha1 = std.crypto.kdf.hkdf.Hkdf(std.crypto.auth.hmac.HmacSha1);
            let prk_arr: [20]u8 = undefined;
            @memcpy(&prk_arr, prk[0..20]);
            HkdfSha1.expand(okm, info, prk_arr);
            return okm;
        },
        .Sha224 => {
            const HkdfSha224 = std.crypto.kdf.hkdf.Hkdf(std.crypto.auth.hmac.sha2.HmacSha224);
            let prk_arr: [28]u8 = undefined;
            @memcpy(&prk_arr, prk[0..28]);
            HkdfSha224.expand(okm, info, prk_arr);
            return okm;
        },
        .Sha384 => {
            const HkdfSha384 = std.crypto.kdf.hkdf.Hkdf(std.crypto.auth.hmac.sha2.HmacSha384);
            let prk_arr: [48]u8 = undefined;
            @memcpy(&prk_arr, prk[0..48]);
            HkdfSha384.expand(okm, info, prk_arr);
            return okm;
        },
        .Blake2b256 => {
            const HkdfB2b = std.crypto.kdf.hkdf.Hkdf(KdfHmacBlake2b256);
            let prk_arr: [32]u8 = undefined;
            @memcpy(&prk_arr, prk[0..32]);
            HkdfB2b.expand(okm, info, prk_arr);
            return okm;
        },
        .Blake2s256 => {
            const HkdfB2s = std.crypto.kdf.hkdf.Hkdf(KdfHmacBlake2s256);
            let prk_arr: [32]u8 = undefined;
            @memcpy(&prk_arr, prk[0..32]);
            HkdfB2s.expand(okm, info, prk_arr);
            return okm;
        },
    }
}

pub fn hkdf(alloc: std.mem.Allocator, alg: HashAlg, salt: []const u8, ikm: []const u8, info: []const u8, out_len: usize): ![]u8 {
    const dlen = digestLen(alg);
    if (out_len > dlen * 255) {
        return ShaError.OutputTooLong;
    }
    const prk = try hkdfExtract(alloc, alg, salt, ikm);
    defer {
        secureZeroBytes(prk);
        alloc.free(prk);
    }
    return try hkdfExpand(alloc, alg, prk, info, out_len);
}

/// Zero-heap RFC 5869 HKDF into caller-provided `out_okm` (enforces `out_okm.len <= 255 * HashLen`).
pub fn hkdfInto(
    alg: HashAlg,
    salt: []const u8,
    ikm: []const u8,
    info: []const u8,
    out_okm: []u8,
): ![]const u8 {
    const dlen = digestLen(alg);
    if (out_okm.len > dlen * 255) return ShaError.OutputTooLong;
    if (out_okm.len == 0) return out_okm;

    let prk_buf: [MAX_DIGEST_LEN]u8 = undefined;
    let mac = StreamingHmac.init(alg, salt);
    mac.update(ikm);
    const prk = try mac.final(&prk_buf);
    defer secureZeroBytes(&prk_buf);

    let t_prev: [MAX_DIGEST_LEN]u8 = undefined;
    let t_len: usize = 0;
    let pos: usize = 0;
    let counter: u8 = 1;
    while (pos < out_okm.len) : (counter += 1) {
        let hmac = StreamingHmac.init(alg, prk);
        if (t_len > 0) hmac.update(t_prev[0..t_len]);
        hmac.update(info);
        const c_arr: [1]u8 = [counter];
        hmac.update(&c_arr);
        const t_cur = try hmac.final(&t_prev);
        t_len = t_cur.len;
        const take = if (out_okm.len - pos < t_len) (out_okm.len - pos) else t_len;
        @memcpy(out_okm[pos .. pos + take], t_cur[0..take]);
        pos += take;
    }
    secureZeroBytes(&t_prev);
    return out_okm;
}

/// 16-lane `@Vector(16, u8)` block XOR for KDF/PBKDF2 accumulation.
pub fn xorBlockSimd(dst: []u8, a: []const u8, b: []const u8) {
    const min_ab = if (a.len < b.len) a.len else b.len;
    const n = if (dst.len < min_ab) dst.len else min_ab;
    let i: usize = 0;
    while (i + 16 <= n) : (i += 16) {
        const va: @Vector(16, u8) = a[i..][0..16].*;
        const vb: @Vector(16, u8) = b[i..][0..16].*;
        dst[i..][0..16].* = va ^ vb;
    }
    while (i < n) : (i += 1) {
        dst[i] = a[i] ^ b[i];
    }
}

/// Scalar reference block XOR for 0-diff parity testing against `xorBlockSimd`.
pub fn xorBlockScalar(dst: []u8, a: []const u8, b: []const u8) {
    const min_ab = if (a.len < b.len) a.len else b.len;
    const n = if (dst.len < min_ab) dst.len else min_ab;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = a[i] ^ b[i];
    }
}

pub fn pbkdf2HmacSha256(alloc: std.mem.Allocator, password: []const u8, salt: []const u8, iterations: u32, out_len: usize): ![]u8 {
    return try pbkdf2Hmac(alloc, .Sha256, password, salt, iterations, out_len);
}

pub fn pbkdf2Hmac(alloc: std.mem.Allocator, alg: HashAlg, password: []const u8, salt: []const u8, iterations: u32, out_len: usize): ![]u8 {
    if (iterations < 1) {
        return ShaError.InvalidIterations;
    }
    let dk = try alloc.alloc(u8, out_len);
    errdefer alloc.free(dk);
    if (out_len == 0) {
        return dk;
    }

    switch (alg) {
        .Sha1 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, std.crypto.auth.hmac.HmacSha1),
        .Sha224 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, std.crypto.auth.hmac.sha2.HmacSha224),
        .Sha256 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, std.crypto.auth.hmac.sha2.HmacSha256),
        .Sha384 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, std.crypto.auth.hmac.sha2.HmacSha384),
        .Sha512 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, std.crypto.auth.hmac.sha2.HmacSha512),
        .Blake2b256 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, KdfHmacBlake2b256),
        .Blake2s256 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, KdfHmacBlake2s256),
    }
    return dk;
}
