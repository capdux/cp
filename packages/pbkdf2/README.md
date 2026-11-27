[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`pbkdf2`](https://github.com/RustCrypto/password-hashes)（`MIT OR Apache-2.0`）（原库名 `pbkdf2`，CP 包名 `@cap/pbkdf2-sha256`）。

# @cap/pbkdf2-sha256

Pure CP, zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification (parity with RustCrypto `pbkdf2` & npm `pbkdf2`).

Reuses streaming SHA-256 and constant-time `ctEqual` from `@cap/cookie-signature`, with precomputed HMAC inner/outer pad states for 2x faster iteration loops.

## API

- `pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, out_dk: []u8): bool`
- `pbkdf2Verify(password: []u8, salt: []u8, iterations: u32, expected_dk: []u8): bool`

## License

MIT
