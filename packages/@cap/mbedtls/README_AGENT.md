# @cap/mbedtls@3.6.2 (Agent Quickref)

> ARM Mbed TLS 3.6.2 core cryptography suite (SHA-256/512, AES-GCM, ChaCha20-Poly1305, Base64) for CP | License: Apache-2.0
> Import: `import { ... } from "@cap/mbedtls";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_mbedtls_version(buf: [*]u8!, cap: usize): usize;`
- `export fn cp_mbedtls_sha256(data: [*]&u8!?, len: usize, out32: [*]u8!): i32;`
- `export fn cp_mbedtls_aes_gcm_roundtrip_test(): i32;`
- `export fn versionInto(buf: []u8): !string;`
- `export fn sha256(data: &[]u8): ![32]u8;`
- `export fn aesGcmRoundtripTest(): bool;`
