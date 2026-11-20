// Source: bun/src/sha_hmac/sha.cp -> cp/lib/sha_hmac/hash.cp (Pure CP unified cryptographic hash primitives)

const std = @import("std");

pub const ShaError = error{
    InvalidLength,
    InvalidIterations,
    OutputTooLong,
    BufferTooSmall,
    OutOfMemory,
};

pub const HashAlg = enum {
    Sha1,
    Sha224,
    Sha256,
    Sha384,
    Sha512,
    Blake2b256,
    Blake2s256,
};

pub const MAX_DIGEST_LEN: usize = 64;

pub fn digestLen(alg: HashAlg): usize {
    return switch (alg) {
        .Sha1 => 20,
        .Sha224 => 28,
        .Sha256 => 32,
        .Sha384 => 48,
        .Sha512 => 64,
        .Blake2b256 => 32,
        .Blake2s256 => 32,
    };
}

pub fn blockLen(alg: HashAlg): usize {
    return switch (alg) {
        .Sha1 => 64,
        .Sha224 => 64,
        .Sha256 => 64,
        .Sha384 => 128,
        .Sha512 => 128,
        .Blake2b256 => 128,
        .Blake2s256 => 64,
    };
}

const HasherState = union(enum) {
    Sha1: std.crypto.hash.Sha1,
    Sha224: std.crypto.hash.sha2.Sha224,
    Sha256: std.crypto.hash.sha2.Sha256,
    Sha384: std.crypto.hash.sha2.Sha384,
    Sha512: std.crypto.hash.sha2.Sha512,
    Blake2b256: std.crypto.hash.blake2.Blake2b256,
    Blake2s256: std.crypto.hash.blake2.Blake2s256,
};

pub struct StreamingHasher {
    alg: HashAlg,
    state: HasherState,

    pub fn init(alg: HashAlg): StreamingHasher {
        const st: HasherState = switch (alg) {
            .Sha1 => HasherState{ .Sha1 = std.crypto.hash.Sha1.init(.{}) },
            .Sha224 => HasherState{ .Sha224 = std.crypto.hash.sha2.Sha224.init(.{}) },
            .Sha256 => HasherState{ .Sha256 = std.crypto.hash.sha2.Sha256.init(.{}) },
            .Sha384 => HasherState{ .Sha384 = std.crypto.hash.sha2.Sha384.init(.{}) },
            .Sha512 => HasherState{ .Sha512 = std.crypto.hash.sha2.Sha512.init(.{}) },
            .Blake2b256 => HasherState{ .Blake2b256 = std.crypto.hash.blake2.Blake2b256.init(.{}) },
            .Blake2s256 => HasherState{ .Blake2s256 = std.crypto.hash.blake2.Blake2s256.init(.{}) },
        };
        return StreamingHasher{
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

    pub fn final(*_, out_buf: []u8): ShaError![]const u8 {
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
        const digest = try _.final(&raw_buf);
        return try digestHex(alloc, digest);
    }
}

pub fn digestHex(alloc: std.mem.Allocator, digest: []const u8): ![]u8 {
    const hex_chars = "0123456789abcdef";
    let out = try alloc.alloc(u8, digest.len * 2);
    let i: usize = 0;
    while (i < digest.len) {
        const b = digest[i];
        out[i * 2] = hex_chars[@as(usize, b >> 4)];
        out[i * 2 + 1] = hex_chars[@as(usize, b & 0x0f)];
        i += 1;
    }
    return out;
}

pub fn hashOneshot(alloc: std.mem.Allocator, alg: HashAlg, data: []const u8): ![]u8 {
    const dlen = digestLen(alg);
    let out = try alloc.alloc(u8, dlen);
    errdefer alloc.free(out);
    let hasher = StreamingHasher.init(alg);
    hasher.update(data);
    * = try hasher.final(out);
    return out;
}

pub fn hashHex(alloc: std.mem.Allocator, alg: HashAlg, data: []const u8): ![]u8 {
    let raw_buf: [MAX_DIGEST_LEN]u8 = undefined;
    let hasher = StreamingHasher.init(alg);
    hasher.update(data);
    const digest = try hasher.final(&raw_buf);
    return try digestHex(alloc, digest);
}
