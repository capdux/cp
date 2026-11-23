// Source: Absorbed from /data/dev/bun/src/boringssl/boringssl.cp | Synced: 2026-11-22T11:07:59Z
// Reference-hardened against RustCrypto/AEADs (MIT/Apache-2.0)
// lib/crypto/cipher.cp
// Authenticated Encryption with Associated Data (AEAD) Ciphers:
// - NIST SP 800-38D AES-256-GCM
// - RFC 8439 ChaCha20-Poly1305
// with 16-lane @Vector(16, u8) constant-time tag comparison, SIMD block XOR,
// zero-heap in-place & combined APIs, and fail-closed plaintext zeroization.

const std = @import("std");

type AeadVec16 = @Vector(16, u8);

pub err AeadError {
    InvalidKeyLength,
    InvalidNonceLength,
    BufferTooSmall,
    CiphertextTooShort,
    AuthenticationFailed,
}

/// Securely zeroizes sensitive key/plaintext memory using `std.crypto.secureZero`.
pub fn aeadSecureZero(buf: []u8) {
    if (buf.len > 0) {
        std.crypto.secureZero(u8, buf);
    }
}

/// 16-lane `@Vector(16, u8)` constant-time 16-byte AEAD authentication tag comparison.
/// Uses `@reduce(.Or, va ^ vb) == 0` with zero data-dependent branching.
pub fn aeadTagEqlSimd(a: *const [16]u8, b: *const [16]u8): bool {
    const va: AeadVec16 = a.*;
    const vb: AeadVec16 = b.*;
    return @reduce(.Or, va ^ vb) == 0;
}

/// 16-lane `@Vector(16, u8)` SIMD block XOR (`out[i] = a[i] ^ b[i]`) for keystream/ciphertext hot paths.
/// Returns `false` if slice lengths do not match.
pub fn aeadXorSimd(out: []u8, a: []const u8, b: []const u8): bool {
    if (out.len != a.len || out.len != b.len) return false;
    let i: usize = 0;
    const n = out.len;
    while (i + 16 <= n) {
        const va: AeadVec16 = a[i..][0..16].*;
        const vb: AeadVec16 = b[i..][0..16].*;
        const xr: [16]u8 = va ^ vb;
        @memcpy(out[i .. i + 16], &xr);
        i += 16;
    }
    while (i < n) : (i += 1) {
        out[i] = a[i] ^ b[i];
    }
    return true;
}

pub class Aes256Gcm {
    pub const KEY_LENGTH: usize = 32;
    pub const NONCE_LENGTH: usize = 12;
    pub const TAG_LENGTH: usize = 16;

    /// Legacy-compatible detached AES-256-GCM encryption.
    pub static fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) {
        if (key.len != 32 || nonce.len != 12 || ciphertext.len < plaintext.len) {
            return;
        }
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes256Gcm.encrypt(
            unsafe { @constCast(ciphertext[0..plaintext.len]) },
            tag,
            plaintext,
            ad,
            n,
            k,
        );
    }

    /// Legacy-compatible detached AES-256-GCM decryption (fail-closed: zeroizes `plaintext` on failure).
    pub static fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ): bool {
        if (key.len != 32 || nonce.len != 12 || plaintext.len < ciphertext.len) {
            aeadSecureZero(plaintext);
            return false;
        }
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        const dst = unsafe { @constCast(plaintext[0..ciphertext.len]) };
        std.crypto.aead.aes_gcm.Aes256Gcm.decrypt(dst, ciphertext, tag, ad, n, k) catch {
            aeadSecureZero(dst);
            return false;
        };
        return true;
    }

    /// Strict detached AES-256-GCM encryption returning explicit `AeadError` on invalid lengths.
    pub static fn encryptDetached(
        key: []const u8,
        nonce: []const u8,
        plaintext: []const u8,
        ad: []const u8,
        ciphertext: []u8,
        tag: *[16]u8,
    ): ! {
        if (key.len != 32) return err.InvalidKeyLength;
        if (nonce.len != 12) return err.InvalidNonceLength;
        if (ciphertext.len < plaintext.len) return err.BufferTooSmall;
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes256Gcm.encrypt(ciphertext[0..plaintext.len], tag, plaintext, ad, n, k);
    }

    /// Strict detached AES-256-GCM decryption; zeroizes `plaintext` before returning `AuthenticationFailed`.
    pub static fn decryptDetached(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: *const [16]u8,
        ad: []const u8,
        plaintext: []u8,
    ): ! {
        if (key.len != 32) return err.InvalidKeyLength;
        if (nonce.len != 12) return err.InvalidNonceLength;
        if (plaintext.len < ciphertext.len) return err.BufferTooSmall;
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        const dst = plaintext[0..ciphertext.len];
        std.crypto.aead.aes_gcm.Aes256Gcm.decrypt(dst, ciphertext, tag.*, ad, n, k) catch {
            aeadSecureZero(dst);
            return err.AuthenticationFailed;
        };
    }

    /// Zero-heap in-place AES-256-GCM encryption (`buf` is overwritten with ciphertext, 16B tag written to `tag`).
    pub static fn encryptInPlace(
        key: []const u8,
        nonce: []const u8,
        ad: []const u8,
        buf: []u8,
        tag: *[16]u8,
    ): ! {
        return Aes256Gcm.encryptDetached(key, nonce, buf, ad, buf, tag);
    }

    /// Zero-heap in-place AES-256-GCM decryption; zeroizes `buf` on authentication failure.
    pub static fn decryptInPlace(
        key: []const u8,
        nonce: []const u8,
        ad: []const u8,
        buf: []u8,
        tag: *const [16]u8,
    ): ! {
        return Aes256Gcm.decryptDetached(key, nonce, buf, tag, ad, buf);
    }

    /// Zero-heap combined seal: writes `ciphertext || tag[16]` into `out` and returns the written slice.
    pub static fn sealCombined(
        key: []const u8,
        nonce: []const u8,
        plaintext: []const u8,
        ad: []const u8,
        out: []u8,
    ): ([]const u8)! {
        const needed = plaintext.len + 16;
        if (out.len < needed) return err.BufferTooSmall;
        let tag: [16]u8 = undefined;
        Aes256Gcm.encryptDetached(key, nonce, plaintext, ad, out[0..plaintext.len], &tag)?;
        @memcpy(out[plaintext.len..needed], &tag);
        return out[0..needed];
    }

    /// Zero-heap combined open: rejects inputs shorter than 16B tag and zeroizes `out` on authentication failure.
    pub static fn openCombined(
        key: []const u8,
        nonce: []const u8,
        combined: []const u8,
        ad: []const u8,
        out: []u8,
    ): ([]const u8)! {
        if (combined.len < 16) return err.CiphertextTooShort;
        const ct_len = combined.len - 16;
        if (out.len < ct_len) return err.BufferTooSmall;
        let tag: [16]u8 = undefined;
        @memcpy(&tag, combined[ct_len..combined.len]);
        Aes256Gcm.decryptDetached(key, nonce, combined[0..ct_len], &tag, ad, out[0..ct_len])?;
        return out[0..ct_len];
    }
}

pub class ChaCha20Poly1305 {
    pub const KEY_LENGTH: usize = 32;
    pub const NONCE_LENGTH: usize = 12;
    pub const TAG_LENGTH: usize = 16;

    /// Legacy-compatible detached RFC 8439 ChaCha20-Poly1305 encryption.
    pub static fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) {
        if (key.len != 32 || nonce.len != 12 || ciphertext.len < plaintext.len) {
            return;
        }
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.encrypt(
            unsafe { @constCast(ciphertext[0..plaintext.len]) },
            tag,
            plaintext,
            ad,
            n,
            k,
        );
    }

    /// Legacy-compatible detached RFC 8439 ChaCha20-Poly1305 decryption (fail-closed: zeroizes `plaintext` on failure).
    pub static fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ): bool {
        if (key.len != 32 || nonce.len != 12 || plaintext.len < ciphertext.len) {
            aeadSecureZero(plaintext);
            return false;
        }
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        const dst = unsafe { @constCast(plaintext[0..ciphertext.len]) };
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.decrypt(dst, ciphertext, tag, ad, n, k) catch {
            aeadSecureZero(dst);
            return false;
        };
        return true;
    }

    /// Strict detached RFC 8439 ChaCha20-Poly1305 encryption returning explicit `AeadError`.
    pub static fn encryptDetached(
        key: []const u8,
        nonce: []const u8,
        plaintext: []const u8,
        ad: []const u8,
        ciphertext: []u8,
        tag: *[16]u8,
    ): ! {
        if (key.len != 32) return err.InvalidKeyLength;
        if (nonce.len != 12) return err.InvalidNonceLength;
        if (ciphertext.len < plaintext.len) return err.BufferTooSmall;
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.encrypt(ciphertext[0..plaintext.len], tag, plaintext, ad, n, k);
    }

    /// Strict detached RFC 8439 ChaCha20-Poly1305 decryption; zeroizes `plaintext` before returning `AuthenticationFailed`.
    pub static fn decryptDetached(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: *const [16]u8,
        ad: []const u8,
        plaintext: []u8,
    ): ! {
        if (key.len != 32) return err.InvalidKeyLength;
        if (nonce.len != 12) return err.InvalidNonceLength;
        if (plaintext.len < ciphertext.len) return err.BufferTooSmall;
        let k: [32]u8 = undefined;
        let n: [12]u8 = undefined;
        defer aeadSecureZero(&k);
        defer aeadSecureZero(&n);
        @memcpy(&k, key[0..32]);
        @memcpy(&n, nonce[0..12]);
        const dst = plaintext[0..ciphertext.len];
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.decrypt(dst, ciphertext, tag.*, ad, n, k) catch {
            aeadSecureZero(dst);
            return err.AuthenticationFailed;
        };
    }

    /// Zero-heap in-place RFC 8439 ChaCha20-Poly1305 encryption.
    pub static fn encryptInPlace(
        key: []const u8,
        nonce: []const u8,
        ad: []const u8,
        buf: []u8,
        tag: *[16]u8,
    ): ! {
        return ChaCha20Poly1305.encryptDetached(key, nonce, buf, ad, buf, tag);
    }

    /// Zero-heap in-place RFC 8439 ChaCha20-Poly1305 decryption; zeroizes `buf` on authentication failure.
    pub static fn decryptInPlace(
        key: []const u8,
        nonce: []const u8,
        ad: []const u8,
        buf: []u8,
        tag: *const [16]u8,
    ): ! {
        return ChaCha20Poly1305.decryptDetached(key, nonce, buf, tag, ad, buf);
    }

    /// Zero-heap combined seal: writes `ciphertext || tag[16]` into `out` and returns the written slice.
    pub static fn sealCombined(
        key: []const u8,
        nonce: []const u8,
        plaintext: []const u8,
        ad: []const u8,
        out: []u8,
    ): ([]const u8)! {
        const needed = plaintext.len + 16;
        if (out.len < needed) return err.BufferTooSmall;
        let tag: [16]u8 = undefined;
        ChaCha20Poly1305.encryptDetached(key, nonce, plaintext, ad, out[0..plaintext.len], &tag)?;
        @memcpy(out[plaintext.len..needed], &tag);
        return out[0..needed];
    }

    /// Zero-heap combined open: rejects inputs shorter than 16B tag and zeroizes `out` on authentication failure.
    pub static fn openCombined(
        key: []const u8,
        nonce: []const u8,
        combined: []const u8,
        ad: []const u8,
        out: []u8,
    ): ([]const u8)! {
        if (combined.len < 16) return err.CiphertextTooShort;
        const ct_len = combined.len - 16;
        if (out.len < ct_len) return err.BufferTooSmall;
        let tag: [16]u8 = undefined;
        @memcpy(&tag, combined[ct_len..combined.len]);
        ChaCha20Poly1305.decryptDetached(key, nonce, combined[0..ct_len], &tag, ad, out[0..ct_len])?;
        return out[0..ct_len];
    }
}
