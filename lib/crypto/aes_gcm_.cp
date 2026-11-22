// Reference Only: Functional overlap with lib/crypto/cipher.cp & lib/crypto/kdf.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (NIST SP 800-38D AES-GCM & AES-GCM-SIV)
const std = @import("std");

pub const Aes128Gcm = std.crypto.aead.aes_gcm.Aes128Gcm;
pub const Aes256Gcm = std.crypto.aead.aes_gcm.Aes256Gcm;
pub const Aes256GcmSiv = std.crypto.aead.aes_gcm_siv.Aes256GcmSiv;

pub const KEY_LEN_128: usize = 16;
pub const KEY_LEN_256: usize = 32;
pub const NONCE_LEN: usize = 12;
pub const TAG_LEN: usize = 16;

pub const AesGcmError = error {
    AuthenticationFailed,
    BufferTooSmall,
};

/// Encrypts `plaintext` with AES-128-GCM into `ciphertext` and writes the 16-byte `tag`.
pub fn encrypt128(
    ciphertext: []u8,
    tag: *[16]u8,
    plaintext: []const u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [16]u8,
): AesGcmError!void {
    if (ciphertext.len < plaintext.len) {
        return error.BufferTooSmall;
    }
    Aes128Gcm.encrypt(ciphertext[0..plaintext.len], tag, plaintext, ad, nonce, key);
}

/// Decrypts `ciphertext` with AES-128-GCM into `plaintext` and verifies the 16-byte `tag`.
pub fn decrypt128(
    plaintext: []u8,
    ciphertext: []const u8,
    tag: [16]u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [16]u8,
): AesGcmError!void {
    if (plaintext.len < ciphertext.len) {
        return error.BufferTooSmall;
    }
    Aes128Gcm.decrypt(plaintext[0..ciphertext.len], ciphertext, tag, ad, nonce, key) catch {
        return error.AuthenticationFailed;
    };
}

/// Encrypts `plaintext` with AES-256-GCM into `ciphertext` and writes the 16-byte `tag`.
pub fn encrypt256(
    ciphertext: []u8,
    tag: *[16]u8,
    plaintext: []const u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [32]u8,
): AesGcmError!void {
    if (ciphertext.len < plaintext.len) {
        return error.BufferTooSmall;
    }
    Aes256Gcm.encrypt(ciphertext[0..plaintext.len], tag, plaintext, ad, nonce, key);
}

/// Decrypts `ciphertext` with AES-256-GCM into `plaintext` and verifies the 16-byte `tag`.
pub fn decrypt256(
    plaintext: []u8,
    ciphertext: []const u8,
    tag: [16]u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [32]u8,
): AesGcmError!void {
    if (plaintext.len < ciphertext.len) {
        return error.BufferTooSmall;
    }
    Aes256Gcm.decrypt(plaintext[0..ciphertext.len], ciphertext, tag, ad, nonce, key) catch {
        return error.AuthenticationFailed;
    };
}

/// Encrypts `plaintext` with RFC 8452 AES-256-GCM-SIV (nonce-misuse resistant).
pub fn encrypt256Siv(
    ciphertext: []u8,
    tag: *[16]u8,
    plaintext: []const u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [32]u8,
): AesGcmError!void {
    if (ciphertext.len < plaintext.len) {
        return error.BufferTooSmall;
    }
    Aes256GcmSiv.encrypt(ciphertext[0..plaintext.len], tag, plaintext, ad, nonce, key);
}

/// Decrypts `ciphertext` with RFC 8452 AES-256-GCM-SIV and verifies the 16-byte `tag`.
pub fn decrypt256Siv(
    plaintext: []u8,
    ciphertext: []const u8,
    tag: [16]u8,
    ad: []const u8,
    nonce: [12]u8,
    key: [32]u8,
): AesGcmError!void {
    if (plaintext.len < ciphertext.len) {
        return error.BufferTooSmall;
    }
    Aes256GcmSiv.decrypt(plaintext[0..ciphertext.len], ciphertext, tag, ad, nonce, key) catch {
        return error.AuthenticationFailed;
    };
}

/// Allocates and returns `nonce (12B) || ciphertext || tag (16B)` using a CSPRNG nonce.
pub fn seal256Alloc(
    alloc: std.mem.Allocator,
    plaintext: []const u8,
    ad: []const u8,
    key: [32]u8,
): ![]u8 {
    const total_len: usize = NONCE_LEN + plaintext.len + TAG_LEN;
    let out = try alloc.alloc(u8, total_len);
    errdefer alloc.free(out);

    let nonce: [12]u8 = undefined;
    let threaded = std.Io.Threaded.init(alloc, .{});
    defer threaded.deinit();
    threaded.io().random(&nonce);

    @memcpy(out[0..NONCE_LEN], &nonce);
    let tag: [16]u8 = undefined;
    const ct_end: usize = NONCE_LEN + plaintext.len;
    Aes256Gcm.encrypt(out[NONCE_LEN..ct_end], &tag, plaintext, ad, nonce, key);
    @memcpy(out[ct_end..total_len], &tag);
    return out;
}

/// Verifies and decrypts a `nonce (12B) || ciphertext || tag (16B)` buffer produced by `seal256Alloc`.
pub fn open256Alloc(
    alloc: std.mem.Allocator,
    sealed: []const u8,
    ad: []const u8,
    key: [32]u8,
): ![]u8 {
    if (sealed.len < NONCE_LEN + TAG_LEN) {
        return error.AuthenticationFailed;
    }
    const ct_len: usize = sealed.len - NONCE_LEN - TAG_LEN;
    const ct_end: usize = NONCE_LEN + ct_len;

    let nonce: [12]u8 = undefined;
    @memcpy(&nonce, sealed[0..NONCE_LEN]);

    let tag: [16]u8 = undefined;
    @memcpy(&tag, sealed[ct_end..sealed.len]);

    let plaintext = try alloc.alloc(u8, ct_len);
    errdefer alloc.free(plaintext);

    Aes256Gcm.decrypt(plaintext, sealed[NONCE_LEN..ct_end], tag, ad, nonce, key) catch {
        return error.AuthenticationFailed;
    };
    return plaintext;
}
