// Source: cp/lib/chacha20/chacha20.cp (Pure CP RFC 8439 ChaCha20-Poly1305 & XChaCha20-Poly1305 AEAD)
const std = @import("std");
const aead = std.crypto.aead.chacha_poly;

pub const KEY_LEN: usize = 32;
pub const NONCE_LEN: usize = 12;
pub const XNONCE_LEN: usize = 24;
pub const TAG_LEN: usize = 16;

pub const ChaChaError = error{
    AuthenticationFailed,
    BufferTooSmall,
};

pub fn encrypt(
    key: [32]u8,
    nonce: [12]u8,
    aad: []const u8,
    plaintext: []const u8,
    ciphertext_out: []u8,
): ChaChaError![16]u8 {
    if (ciphertext_out.len < plaintext.len) {
        return ChaChaError.BufferTooSmall;
    }
    let tag: [16]u8 = undefined;
    aead.ChaCha20Poly1305.encrypt(ciphertext_out[0..plaintext.len], &tag, plaintext, aad, nonce, key);
    return tag;
}

pub fn decrypt(
    key: [32]u8,
    nonce: [12]u8,
    aad: []const u8,
    ciphertext: []const u8,
    tag: [16]u8,
    plaintext_out: []u8,
): ChaChaError![]u8 {
    if (plaintext_out.len < ciphertext.len) {
        return ChaChaError.BufferTooSmall;
    }
    const dst = plaintext_out[0..ciphertext.len];
    aead.ChaCha20Poly1305.decrypt(dst, ciphertext, tag, aad, nonce, key) catch {
        return ChaChaError.AuthenticationFailed;
    };
    return dst;
}

pub fn encryptX(
    key: [32]u8,
    nonce: [24]u8,
    aad: []const u8,
    plaintext: []const u8,
    ciphertext_out: []u8,
): ChaChaError![16]u8 {
    if (ciphertext_out.len < plaintext.len) {
        return ChaChaError.BufferTooSmall;
    }
    let tag: [16]u8 = undefined;
    aead.XChaCha20Poly1305.encrypt(ciphertext_out[0..plaintext.len], &tag, plaintext, aad, nonce, key);
    return tag;
}

pub fn decryptX(
    key: [32]u8,
    nonce: [24]u8,
    aad: []const u8,
    ciphertext: []const u8,
    tag: [16]u8,
    plaintext_out: []u8,
): ChaChaError![]u8 {
    if (plaintext_out.len < ciphertext.len) {
        return ChaChaError.BufferTooSmall;
    }
    const dst = plaintext_out[0..ciphertext.len];
    aead.XChaCha20Poly1305.decrypt(dst, ciphertext, tag, aad, nonce, key) catch {
        return ChaChaError.AuthenticationFailed;
    };
    return dst;
}

/// Seal into nonce[24] || ciphertext || tag[16] using CSPRNG random 24-byte XChaCha20 nonce
pub fn sealXAlloc(
    alloc: std.mem.Allocator,
    key: [32]u8,
    aad: []const u8,
    plaintext: []const u8,
): ![]u8 {
    const total = XNONCE_LEN + plaintext.len + TAG_LEN;
    let out = try alloc.alloc(u8, total);
    errdefer alloc.free(out);

    let threaded = std.Io.Threaded.init(alloc, .{});
    defer threaded.deinit();
    let nonce: [24]u8 = undefined;
    threaded.io().random(&nonce);
    @memcpy(out[0..XNONCE_LEN], &nonce);

    let tag: [16]u8 = undefined;
    aead.XChaCha20Poly1305.encrypt(out[XNONCE_LEN .. XNONCE_LEN + plaintext.len], &tag, plaintext, aad, nonce, key);
    @memcpy(out[XNONCE_LEN + plaintext.len .. total], &tag);
    return out;
}

pub fn openXAlloc(
    alloc: std.mem.Allocator,
    key: [32]u8,
    aad: []const u8,
    sealed: []const u8,
): ![]u8 {
    if (sealed.len < XNONCE_LEN + TAG_LEN) {
        return ChaChaError.AuthenticationFailed;
    }
    const ct_len = sealed.len - XNONCE_LEN - TAG_LEN;
    let nonce: [24]u8 = undefined;
    @memcpy(&nonce, sealed[0..XNONCE_LEN]);
    let tag: [16]u8 = undefined;
    @memcpy(&tag, sealed[XNONCE_LEN + ct_len .. sealed.len]);

    const pt = try alloc.alloc(u8, ct_len);
    errdefer alloc.free(pt);
    aead.XChaCha20Poly1305.decrypt(pt, sealed[XNONCE_LEN .. XNONCE_LEN + ct_len], tag, aad, nonce, key) catch {
        return ChaChaError.AuthenticationFailed;
    };
    return pt;
}
