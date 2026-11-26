# @cap/chacha20-poly1305

Pure CP, zero-allocation RFC 8439 ChaCha20-Poly1305 and draft-irtf-cfrg-xchacha XChaCha20-Poly1305 AEAD cipher (parity with RustCrypto `chacha20poly1305`).

Reuses `@cap/poly1305` for Poly1305 authentication, `@cap/chacha-rng` for the RFC 8439 ChaCha20 core block function, and `@cap/subtle-ct` for constant-time tag verification and sensitive buffer zeroization.

## API

### Constants
- `CHACHA20_KEY_LEN` (`32`)
- `CHACHA20_NONCE_LEN` (`12`)
- `CHACHA20_BLOCK_LEN` (`64`)
- `CHACHA20_TAG_LEN` (`16`)
- `XCHACHA20_NONCE_LEN` (`24`)

### ChaCha20 & Poly1305 Key Generation (RFC 8439)
- `chacha20Block(key: []const u8, counter: u32, nonce: []const u8, out: []u8): bool`
- `chacha20Xor(key: []const u8, counter: u32, nonce: []const u8, input: []const u8, out: []u8): bool`
- `poly1305KeyGen(key: []const u8, nonce: []const u8, out: []u8): bool`

### ChaCha20-Poly1305 AEAD (RFC 8439 §2.8)
- `aeadEncrypt(key: []const u8, nonce: []const u8, aad: []const u8, plaintext: []const u8, ciphertext: []u8, tag: []u8): bool`
- `aeadDecrypt(key: []const u8, nonce: []const u8, aad: []const u8, ciphertext: []const u8, tag: []const u8, plaintext: []u8): bool`

### XChaCha20-Poly1305 Extended 192-bit Nonce AEAD
- `xchacha20HChaCha20(key: []const u8, nonce16: []const u8, out_subkey: []u8): bool`
- `xchacha20Poly1305Encrypt(key: []const u8, nonce24: []const u8, aad: []const u8, plaintext: []const u8, ciphertext: []u8, tag: []u8): bool`
- `xchacha20Poly1305Decrypt(key: []const u8, nonce24: []const u8, aad: []const u8, ciphertext: []const u8, tag: []const u8, plaintext: []u8): bool`

## License

MIT
