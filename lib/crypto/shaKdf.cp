// Source: cp/lib/sha_hmac/kdf.cp (Pure CP RFC 5869 HKDF & RFC 2898 PBKDF2-HMAC)

const std = @import("std");
import { HashAlg, ShaError, digestLen } from "./shaHash.cp";
import { StreamingHmac } from "./shaHmac.cp";

const HmacBlake2b256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2b256);
const HmacBlake2s256 = std.crypto.auth.hmac.Hmac(std.crypto.hash.blake2.Blake2s256);

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
            const HkdfB2b = std.crypto.kdf.hkdf.Hkdf(HmacBlake2b256);
            let prk_arr: [32]u8 = undefined;
            @memcpy(&prk_arr, prk[0..32]);
            HkdfB2b.expand(okm, info, prk_arr);
            return okm;
        },
        .Blake2s256 => {
            const HkdfB2s = std.crypto.kdf.hkdf.Hkdf(HmacBlake2s256);
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
    defer alloc.free(prk);
    return try hkdfExpand(alloc, alg, prk, info, out_len);
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
        .Blake2b256 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, HmacBlake2b256),
        .Blake2s256 => try std.crypto.pwhash.pbkdf2(dk, password, salt, iterations, HmacBlake2s256),
    }
    return dk;
}
