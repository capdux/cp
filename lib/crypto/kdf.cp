// Source: Absorbed from /data/dev/bun/src/sha_hmac/hmac.cp | Synced: 2026-11-22T12:49:17Z
// lib/crypto/kdf.cp
// Key Derivation (HKDF, Scrypt, Argon2id), Modern Hashing (BLAKE3, SHA-384, SHA3-256/512), HMAC-SHA384/512 & AEAD (XChaCha20-Poly1305, AES-128-GCM)

import { bytesToHex } from "./pbkdf2.cp";

// =============================================================================
// 1. HKDF-SHA256 & HKDF-SHA512 (RFC 5869)
// =============================================================================

pub fn hkdfExtract(salt: string, ikm: string): [32]u8 {
    return std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
}

pub fn hkdfExpand(out: []u8, info: string, prk: []const u8): bool {
    if (prk.len != 32 || out.len > 32 * 255) return false;
    let prk_arr: [32]u8 = undefined;
    @memcpy(&prk_arr, prk[0..32]);
    std.crypto.kdf.hkdf.HkdfSha256.expand(out, info, prk_arr);
    return true;
}

pub fn hkdfDerive(out: []u8, ikm: string, salt: string, info: string): bool {
    if (out.len > 32 * 255) return false;
    const prk = std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
    std.crypto.kdf.hkdf.HkdfSha256.expand(out, info, prk);
    return true;
}

pub fn hkdfDeriveAlloc(ikm: string, salt: string, info: string, length: usize): ([]u8)? {
    if (length > 32 * 255) return null;
    const out = std.heap.page_allocator.alloc(u8, length) catch return null;
    const prk = std.crypto.kdf.hkdf.HkdfSha256.extract(salt, ikm);
    std.crypto.kdf.hkdf.HkdfSha256.expand(out, info, prk);
    return out;
}

pub fn hkdfSha512Extract(salt: string, ikm: string): [64]u8 {
    return std.crypto.kdf.hkdf.HkdfSha512.extract(salt, ikm);
}

pub fn hkdfSha512Expand(out: []u8, info: string, prk: []const u8): bool {
    if (prk.len != 64 || out.len > 64 * 255) return false;
    let prk_arr: [64]u8 = undefined;
    @memcpy(&prk_arr, prk[0..64]);
    std.crypto.kdf.hkdf.HkdfSha512.expand(out, info, prk_arr);
    return true;
}

pub fn hkdfSha512Derive(out: []u8, ikm: string, salt: string, info: string): bool {
    if (out.len > 64 * 255) return false;
    const prk = std.crypto.kdf.hkdf.HkdfSha512.extract(salt, ikm);
    std.crypto.kdf.hkdf.HkdfSha512.expand(out, info, prk);
    return true;
}

// =============================================================================
// 2. Scrypt (RFC 7914) & Argon2id (RFC 9106) Password Hashing
// =============================================================================

pub fn scryptDerive(out: []u8, password: string, salt: string, log2_n: u6, r: u30, p: u30): bool {
    std.crypto.pwhash.scrypt.kdf(
        std.heap.page_allocator,
        out,
        password,
        salt,
        std.crypto.pwhash.scrypt.Params{ ln: log2_n, r: r, p: p },
    ) catch return false;
    return true;
}

pub fn scryptDeriveAlloc(password: string, salt: string, keylen: usize, log2_n: u6, r: u30, p: u30): ([]u8)? {
    const out = std.heap.page_allocator.alloc(u8, keylen) catch return null;
    if (!scryptDerive(out, password, salt, log2_n, r, p)) {
        std.heap.page_allocator.free(out);
        return null;
    }
    return out;
}

pub fn argon2idDerive(out: []u8, password: string, salt: string, t: u32, m: u32, p: u24): bool {
    zig {
        var threaded: std.Io.Threaded = .init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        std.crypto.pwhash.argon2.kdf(
            std.heap.page_allocator,
            out,
            password,
            salt,
            .{ .t = t, .m = m, .p = p },
            .argon2id,
            threaded.io(),
        ) catch return false;
        return true;
    }
}

pub fn argon2idHash(password: string, t: u32, m: u32, p: u24): string? {
    zig {
        var threaded: std.Io.Threaded = .init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        var buf: [256]u8 = undefined;
        const res = std.crypto.pwhash.argon2.strHash(
            password,
            .{
                .allocator = std.heap.page_allocator,
                .params = .{ .t = t, .m = m, .p = p },
                .mode = .argon2id,
                .encoding = .phc,
            },
            &buf,
            threaded.io(),
        ) catch return null;
        const owned = std.heap.page_allocator.alloc(u8, res.len) catch return null;
        @memcpy(owned, res);
        return owned;
    }
}

pub fn argon2idVerify(encoded_hash: string, password: string): bool {
    zig {
        var threaded: std.Io.Threaded = .init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        std.crypto.pwhash.argon2.strVerify(
            encoded_hash,
            password,
            .{ .allocator = std.heap.page_allocator },
            threaded.io(),
        ) catch return false;
        return true;
    }
}

// =============================================================================
// 3. BLAKE3 (One-Shot & Streaming)
// =============================================================================

pub class Blake3Stream {
    pub hasher: std.crypto.hash.Blake3 = undefined;

    pub static fn init(): *Blake3Stream {
        let s = std.heap.page_allocator.create(Blake3Stream) catch unreachable;
        s.* = Blake3Stream{};
        zig {
            s.hasher = std.crypto.hash.Blake3.init(.{});
        }
        return s;
    }

    pub static fn initKeyed(key: []const u8): (*Blake3Stream)? {
        if (key.len != 32) return null;
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let s = std.heap.page_allocator.create(Blake3Stream) catch unreachable;
        s.* = Blake3Stream{};
        zig {
            s.hasher = std.crypto.hash.Blake3.init(.{ .key = k });
        }
        return s;
    }

    pub fn update(*_, data: string): *Blake3Stream {
        _.hasher.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: []u8) {
        _.hasher.final(out);
    }

    pub fn digest(*_): string {
        let out: [32]u8 = undefined;
        _.hasher.final(&out);
        return bytesToHex(&out);
    }
}

pub struct Blake3 {
    const DIGEST_LENGTH: usize = 32;
    const KEY_LENGTH: usize = 32;

    static fn hashRaw(out: []u8, data: string) {
        zig {
            std.crypto.hash.Blake3.hash(data, out, .{});
        }
    }

    static fn hash(data: string): string {
        let out: [32]u8 = undefined;
        Blake3.hashRaw(&out, data);
        return bytesToHex(&out);
    }

    static fn keyed(key: []const u8, data: string): string? {
        if (key.len != 32) return null;
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let out: [32]u8 = undefined;
        zig {
            std.crypto.hash.Blake3.hash(data, &out, .{ .key = k });
        }
        return bytesToHex(&out);
    }

    static fn init(): *Blake3Stream {
        return Blake3Stream.init();
    }
}

pub fn blake3Raw(out: []u8, data: string) {
    Blake3.hashRaw(out, data);
}

pub fn blake3(data: string): string {
    return Blake3.hash(data);
}

// =============================================================================
// 4. SHA-384, SHA3-256 & SHA3-512
// =============================================================================

pub class Sha384 {
    pub const DIGEST_LENGTH: usize = 48;
    pub hasher: std.crypto.hash.sha2.Sha384 = undefined;

    pub static fn hashRaw(out: *[48]u8, data: string) {
        zig {
            std.crypto.hash.sha2.Sha384.hash(data, out, .{});
        }
    }

    pub static fn hash(data: string): string {
        let out: [48]u8 = undefined;
        Sha384.hashRaw(&out, data);
        return bytesToHex(&out);
    }

    pub static fn init(): *Sha384 {
        let h = std.heap.page_allocator.create(Sha384) catch unreachable;
        h.* = Sha384{};
        zig {
            h.hasher = std.crypto.hash.sha2.Sha384.init(.{});
        }
        return h;
    }

    pub fn update(*_, data: string): *Sha384 {
        _.hasher.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: *[48]u8) {
        _.hasher.final(out);
    }

    pub fn digest(*_): string {
        let out: [48]u8 = undefined;
        _.hasher.final(&out);
        return bytesToHex(&out);
    }
}

pub class Sha3_256 {
    pub const DIGEST_LENGTH: usize = 32;
    pub hasher: std.crypto.hash.sha3.Sha3_256 = undefined;

    pub static fn hashRaw(out: *[32]u8, data: string) {
        zig {
            std.crypto.hash.sha3.Sha3_256.hash(data, out, .{});
        }
    }

    pub static fn hash(data: string): string {
        let out: [32]u8 = undefined;
        Sha3_256.hashRaw(&out, data);
        return bytesToHex(&out);
    }

    pub static fn init(): *Sha3_256 {
        let h = std.heap.page_allocator.create(Sha3_256) catch unreachable;
        h.* = Sha3_256{};
        zig {
            h.hasher = std.crypto.hash.sha3.Sha3_256.init(.{});
        }
        return h;
    }

    pub fn update(*_, data: string): *Sha3_256 {
        _.hasher.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: *[32]u8) {
        _.hasher.final(out);
    }

    pub fn digest(*_): string {
        let out: [32]u8 = undefined;
        _.hasher.final(&out);
        return bytesToHex(&out);
    }
}

pub class Sha3_512 {
    pub const DIGEST_LENGTH: usize = 64;
    pub hasher: std.crypto.hash.sha3.Sha3_512 = undefined;

    pub static fn hashRaw(out: *[64]u8, data: string) {
        zig {
            std.crypto.hash.sha3.Sha3_512.hash(data, out, .{});
        }
    }

    pub static fn hash(data: string): string {
        let out: [64]u8 = undefined;
        Sha3_512.hashRaw(&out, data);
        return bytesToHex(&out);
    }

    pub static fn init(): *Sha3_512 {
        let h = std.heap.page_allocator.create(Sha3_512) catch unreachable;
        h.* = Sha3_512{};
        zig {
            h.hasher = std.crypto.hash.sha3.Sha3_512.init(.{});
        }
        return h;
    }

    pub fn update(*_, data: string): *Sha3_512 {
        _.hasher.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: *[64]u8) {
        _.hasher.final(out);
    }

    pub fn digest(*_): string {
        let out: [64]u8 = undefined;
        _.hasher.final(&out);
        return bytesToHex(&out);
    }
}

pub fn sha384Raw(out: *[48]u8, data: string) {
    Sha384.hashRaw(out, data);
}

pub fn sha384(data: string): string {
    return Sha384.hash(data);
}

pub fn sha3_256Raw(out: *[32]u8, data: string) {
    Sha3_256.hashRaw(out, data);
}

pub fn sha3_256(data: string): string {
    return Sha3_256.hash(data);
}

pub fn sha3_512Raw(out: *[64]u8, data: string) {
    Sha3_512.hashRaw(out, data);
}

pub fn sha3_512(data: string): string {
    return Sha3_512.hash(data);
}

// =============================================================================
// 5. HMAC-SHA384 & HMAC-SHA512
// =============================================================================

pub class HmacSha384 {
    pub const MAC_LENGTH: usize = 48;
    pub ctx: std.crypto.auth.hmac.sha2.HmacSha384 = undefined;

    pub static fn createRaw(out: *[48]u8, key: string, data: string) {
        std.crypto.auth.hmac.sha2.HmacSha384.create(out, data, key);
    }

    pub static fn create(key: string, data: string): string {
        let out: [48]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha384.create(&out, data, key);
        return bytesToHex(&out);
    }

    pub static fn init(key: string): *HmacSha384 {
        let h = std.heap.page_allocator.create(HmacSha384) catch unreachable;
        h.* = HmacSha384{};
        h.ctx = std.crypto.auth.hmac.sha2.HmacSha384.init(key);
        return h;
    }

    pub fn update(*_, data: string): *HmacSha384 {
        _.ctx.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: *[48]u8) {
        _.ctx.final(out);
    }

    pub fn digest(*_): string {
        let out: [48]u8 = undefined;
        _.ctx.final(&out);
        return bytesToHex(&out);
    }
}

pub class HmacSha512 {
    pub const MAC_LENGTH: usize = 64;
    pub ctx: std.crypto.auth.hmac.sha2.HmacSha512 = undefined;

    pub static fn createRaw(out: *[64]u8, key: string, data: string) {
        std.crypto.auth.hmac.sha2.HmacSha512.create(out, data, key);
    }

    pub static fn create(key: string, data: string): string {
        let out: [64]u8 = undefined;
        std.crypto.auth.hmac.sha2.HmacSha512.create(&out, data, key);
        return bytesToHex(&out);
    }

    pub static fn init(key: string): *HmacSha512 {
        let h = std.heap.page_allocator.create(HmacSha512) catch unreachable;
        h.* = HmacSha512{};
        h.ctx = std.crypto.auth.hmac.sha2.HmacSha512.init(key);
        return h;
    }

    pub fn update(*_, data: string): *HmacSha512 {
        _.ctx.update(data);
        return _;
    }

    pub fn finalRaw(*_, out: *[64]u8) {
        _.ctx.final(out);
    }

    pub fn digest(*_): string {
        let out: [64]u8 = undefined;
        _.ctx.final(&out);
        return bytesToHex(&out);
    }
}

pub fn hmacSha384Raw(out: *[48]u8, key: string, data: string) {
    HmacSha384.createRaw(out, key, data);
}

pub fn hmacSha384(key: string, data: string): string {
    return HmacSha384.create(key, data);
}

// =============================================================================
// 6. Modern AEAD Ciphers: XChaCha20-Poly1305 (24B Nonce) & AES-128-GCM
// =============================================================================

pub class XChaCha20Poly1305 {
    pub const KEY_LENGTH: usize = 32;
    pub const NONCE_LENGTH: usize = 24;
    pub const TAG_LENGTH: usize = 16;

    pub static fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) {
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [24]u8 = undefined;
        @memcpy(&n, nonce[0..24]);
        std.crypto.aead.chacha_poly.XChaCha20Poly1305.encrypt(unsafe { @constCast(ciphertext) }, tag, plaintext, ad, n, k);
    }

    pub static fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ): bool {
        if (key.len < 32 || nonce.len < 24 || plaintext.len < ciphertext.len) return false;
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [24]u8 = undefined;
        @memcpy(&n, nonce[0..24]);
        std.crypto.aead.chacha_poly.XChaCha20Poly1305.decrypt(unsafe { @constCast(plaintext) }, ciphertext, tag, ad, n, k) catch return false;
        return true;
    }
}

pub class Aes128Gcm {
    pub const KEY_LENGTH: usize = 16;
    pub const NONCE_LENGTH: usize = 12;
    pub const TAG_LENGTH: usize = 16;

    pub static fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) {
        let k: [16]u8 = undefined;
        @memcpy(&k, key[0..16]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes128Gcm.encrypt(unsafe { @constCast(ciphertext) }, tag, plaintext, ad, n, k);
    }

    pub static fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ): bool {
        if (key.len < 16 || nonce.len < 12 || plaintext.len < ciphertext.len) return false;
        let k: [16]u8 = undefined;
        @memcpy(&k, key[0..16]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes128Gcm.decrypt(unsafe { @constCast(plaintext) }, ciphertext, tag, ad, n, k) catch return false;
        return true;
    }
}
