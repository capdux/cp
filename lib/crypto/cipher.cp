import std from "std";

pub const Aes256Gcm = struct {
    pub const KEY_LENGTH: usize = 32;
    pub const NONCE_LENGTH: usize = 12;
    pub const TAG_LENGTH: usize = 16;

    pub fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) void {
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes256Gcm.encrypt(@constCast(ciphertext), tag, plaintext, ad, n, k);
    }

    pub fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ) bool {
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.aes_gcm.Aes256Gcm.decrypt(@constCast(plaintext), ciphertext, tag, ad, n, k) catch return false;
        return true;
    }
};

pub const ChaCha20Poly1305 = struct {
    pub const KEY_LENGTH: usize = 32;
    pub const NONCE_LENGTH: usize = 12;
    pub const TAG_LENGTH: usize = 16;

    pub fn encrypt(
        key: []const u8,
        nonce: []const u8,
        plaintext: string,
        ad: string,
        ciphertext: []u8,
        tag: *[16]u8,
    ) void {
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.encrypt(@constCast(ciphertext), tag, plaintext, ad, n, k);
    }

    pub fn decrypt(
        key: []const u8,
        nonce: []const u8,
        ciphertext: []const u8,
        tag: [16]u8,
        ad: string,
        plaintext: []u8,
    ) bool {
        let k: [32]u8 = undefined;
        @memcpy(&k, key[0..32]);
        let n: [12]u8 = undefined;
        @memcpy(&n, nonce[0..12]);
        std.crypto.aead.chacha_poly.ChaCha20Poly1305.decrypt(@constCast(plaintext), ciphertext, tag, ad, n, k) catch return false;
        return true;
    }
};
