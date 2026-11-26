# @cap/pbkdf2-sha256

Pure CP, zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification (parity with RustCrypto `pbkdf2` & npm `pbkdf2`).

Reuses streaming SHA-256 and constant-time `ctEqual` from `@cap/cookie-signature`, with precomputed HMAC inner/outer pad states for 2x faster iteration loops.

## API

- `pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, out_dk: []u8): bool`
- `pbkdf2Verify(password: []u8, salt: []u8, iterations: u32, expected_dk: []u8): bool`

## License

MIT
