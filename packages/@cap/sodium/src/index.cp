// @cap/sodium — libsodium 1.0.20 modern cryptography suite for CP
// License: ISC (Upstream: Frank Denis / jedisct1/libsodium)

import std from "std";

pub const ED25519_PUBLICKEYBYTES: usize = 32;
pub const ED25519_SECRETKEYBYTES: usize = 64;
pub const ED25519_BYTES: usize = 64;

pub const AEAD_XCHACHA20POLY1305_KEYBYTES: usize = 32;
pub const AEAD_XCHACHA20POLY1305_NPUBBYTES: usize = 24;
pub const AEAD_XCHACHA20POLY1305_ABYTES: usize = 16;

pub const GENERICHASH_BYTES_MIN: usize = 16;
pub const GENERICHASH_BYTES_MAX: usize = 64;
pub const GENERICHASH_BYTES: usize = 32;

pub @c {
    fn sodium_init(): i32;
    fn sodium_version_string(): [*:0]const u8?;
    fn randombytes_buf(buf: [*]u8!, size: usize);

    fn crypto_sign_ed25519_keypair(pk: [*]u8!, sk: [*]u8!): i32;
    fn crypto_sign_ed25519_detached(
        sig: [*]u8!,
        siglen_p: *u64?!,
        m: [*]const u8?!,
        mlen: u64,
        sk: [*]const u8!
    ): i32;
    fn crypto_sign_ed25519_verify_detached(
        sig: [*]const u8!,
        m: [*]const u8?!,
        mlen: u64,
        pk: [*]const u8!
    ): i32;

    fn crypto_aead_xchacha20poly1305_ietf_encrypt(
        c: [*]u8!,
        clen_p: *u64?!,
        m: [*]const u8?!,
        mlen: u64,
        ad: [*]const u8?!,
        adlen: u64,
        nsec: [*]const u8?!,
        npub: [*]const u8!,
        k: [*]const u8!
    ): i32;

    fn crypto_aead_xchacha20poly1305_ietf_decrypt(
        m: [*]u8?!,
        mlen_p: *u64?!,
        nsec: [*]u8?!,
        c: [*]const u8!,
        clen: u64,
        ad: [*]const u8?!,
        adlen: u64,
        npub: [*]const u8!,
        k: [*]const u8!
    ): i32;

    fn crypto_generichash(
        out: [*]u8!,
        outlen: usize,
        in_buf: [*]const u8?!,
        inlen: u64,
        key: [*]const u8?!,
        keylen: usize
    ): i32;
}

pub err SodiumError {
    InitFailed,
    InvalidKeyLength,
    InvalidNonceLength,
    InvalidSignatureLength,
    InvalidHashLength,
    BufferTooSmall,
    CiphertextTooShort,
    KeypairFailed,
    SignFailed,
    EncryptFailed,
    AuthFailed,
    HashFailed,
}

/// Ed25519 keypair containing a 32-byte public key and 64-byte secret key.
pub struct Ed25519Keypair {
    pub pk: [32]u8,
    pub sk: [64]u8,
}

/// Detached 64-byte Ed25519 signature.
pub struct Ed25519Signature {
    pub bytes: [64]u8,
}

/// Safe high-level libsodium 1.0.20 facade.
pub struct Sodium {
    /// Initializes libsodium. Safe to call multiple times (idempotent).
    pub static fn init(): void! {
        let rc: i32 = 0;
        unsafe {
            rc = sodium_init();
        }
        if (rc < 0) return SodiumError.InitFailed;
    }

    /// Returns the linked libsodium version string (e.g. "1.0.20").
    pub static fn versionString(): string {
        unsafe {
            if (ver := sodium_version_string()) {
                return std.mem.span(ver);
            }
        }
        return "1.0.20";
    }

    /// Fills `out` with cryptographically secure random bytes.
    pub static fn randombytesBuf(out: []u8) {
        if (out.len == 0) return;
        unsafe {
            randombytes_buf(out.ptr, out.len);
        }
    }

    /// Generates a fresh Ed25519 public/secret keypair.
    pub static fn ed25519Keypair(): Ed25519Keypair! {
        let kp = Ed25519Keypair{
            .pk = [0; 32],
            .sk = [0; 64],
        };
        let rc: i32 = 0;
        unsafe {
            rc = crypto_sign_ed25519_keypair(&kp.pk, &kp.sk);
        }
        if (rc != 0) return SodiumError.KeypairFailed;
        return kp;
    }

    /// Creates a 64-byte detached Ed25519 signature for `msg` using 64-byte `sk`.
    pub static fn ed25519SignDetached(msg: []const u8, sk: []const u8): Ed25519Signature! {
        if (sk.len != ED25519_SECRETKEYBYTES) return SodiumError.InvalidKeyLength;
        let sig = Ed25519Signature{
            .bytes = [0; 64],
        };
        let sig_len: u64 = 0;
        let rc: i32 = 0;
        unsafe {
            const msg_ptr: [*]const u8?! = if (msg.len == 0) null else msg.ptr;
            rc = crypto_sign_ed25519_detached(
                &sig.bytes,
                &sig_len,
                msg_ptr,
                @intCast(msg.len),
                sk.ptr
            );
        }
        if (rc != 0) return SodiumError.SignFailed;
        return sig;
    }

    /// Verifies a 64-byte detached Ed25519 `sig` over `msg` against 32-byte `pk`.
    /// Returns `true` if authentic, `false` if tampered or invalid.
    pub static fn ed25519VerifyDetached(sig: []const u8, msg: []const u8, pk: []const u8): bool {
        if (sig.len != ED25519_BYTES) return false;
        if (pk.len != ED25519_PUBLICKEYBYTES) return false;
        let rc: i32 = -1;
        unsafe {
            const msg_ptr: [*]const u8?! = if (msg.len == 0) null else msg.ptr;
            rc = crypto_sign_ed25519_verify_detached(
                sig.ptr,
                msg_ptr,
                @intCast(msg.len),
                pk.ptr
            );
        }
        return rc == 0;
    }

    /// Encrypts `msg` with XChaCha20-Poly1305-IETF AEAD using 24-byte `nonce`, 32-byte `key`,
    /// and optional associated data `aad`. Writes `msg.len + 16` bytes into `out` and returns the slice.
    pub static fn aeadEncrypt(
        msg: []const u8,
        aad: []const u8,
        nonce: []const u8,
        key: []const u8,
        out: []u8
    ): []const u8! {
        if (nonce.len != AEAD_XCHACHA20POLY1305_NPUBBYTES) return SodiumError.InvalidNonceLength;
        if (key.len != AEAD_XCHACHA20POLY1305_KEYBYTES) return SodiumError.InvalidKeyLength;
        const needed = msg.len + AEAD_XCHACHA20POLY1305_ABYTES;
        if (out.len < needed) return SodiumError.BufferTooSmall;
        let clen: u64 = 0;
        let rc: i32 = 0;
        unsafe {
            const msg_ptr: [*]const u8?! = if (msg.len == 0) null else msg.ptr;
            const aad_ptr: [*]const u8?! = if (aad.len == 0) null else aad.ptr;
            rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
                out.ptr,
                &clen,
                msg_ptr,
                @intCast(msg.len),
                aad_ptr,
                @intCast(aad.len),
                null,
                nonce.ptr,
                key.ptr
            );
        }
        if (rc != 0) return SodiumError.EncryptFailed;
        return out[0..@intCast(clen)];
    }

    /// Decrypts and authenticates XChaCha20-Poly1305-IETF ciphertext `ct` into `out`.
    /// Returns `SodiumError.AuthFailed` if tag, ciphertext, or AAD verification fails.
    pub static fn aeadDecrypt(
        ct: []const u8,
        aad: []const u8,
        nonce: []const u8,
        key: []const u8,
        out: []u8
    ): []const u8! {
        if (nonce.len != AEAD_XCHACHA20POLY1305_NPUBBYTES) return SodiumError.InvalidNonceLength;
        if (key.len != AEAD_XCHACHA20POLY1305_KEYBYTES) return SodiumError.InvalidKeyLength;
        if (ct.len < AEAD_XCHACHA20POLY1305_ABYTES) return SodiumError.CiphertextTooShort;
        const plain_len = ct.len - AEAD_XCHACHA20POLY1305_ABYTES;
        if (out.len < plain_len) return SodiumError.BufferTooSmall;
        let mlen: u64 = 0;
        let rc: i32 = 0;
        unsafe {
            const out_ptr: [*]u8?! = if (plain_len == 0) null else out.ptr;
            const aad_ptr: [*]const u8?! = if (aad.len == 0) null else aad.ptr;
            rc = crypto_aead_xchacha20poly1305_ietf_decrypt(
                out_ptr,
                &mlen,
                null,
                ct.ptr,
                @intCast(ct.len),
                aad_ptr,
                @intCast(aad.len),
                nonce.ptr,
                key.ptr
            );
        }
        if (rc != 0) return SodiumError.AuthFailed;
        return out[0..@intCast(mlen)];
    }

    /// Computes an unkeyed BLAKE2b digest of `msg` into `out` (`out.len` must be 16..64 bytes).
    pub static fn generichash(msg: []const u8, out: []u8): void! {
        if (out.len < GENERICHASH_BYTES_MIN or out.len > GENERICHASH_BYTES_MAX) {
            return SodiumError.InvalidHashLength;
        }
        let rc: i32 = 0;
        unsafe {
            const msg_ptr: [*]const u8?! = if (msg.len == 0) null else msg.ptr;
            rc = crypto_generichash(
                out.ptr,
                out.len,
                msg_ptr,
                @intCast(msg.len),
                null,
                0
            );
        }
        if (rc != 0) return SodiumError.HashFailed;
    }
}
