# `@cap/block-padding`

Zero-allocation cryptographic block padding & cipher mode primitives in pure CP (RustCrypto `block-padding` + `cipher` parity). Reuses `@cap/subtle-ct` for constant-time padding-oracle-resistant verification and buffer zeroization.

## API

- `pkcs7Pad(input: []const u8, block_size: usize, out: []u8): i64` — RFC 5652 §6.3 PKCS#7 padding (`1 <= block_size <= 255`)
- `pkcs7UnpadCt(padded: []const u8, block_size: usize): i64` — Branchless constant-time PKCS#7 unpadding via `@cap/subtle-ct`
- `iso7816Pad` / `iso7816Unpad` — ISO/IEC 7816-4 (`0x80` + `0x00*`)
- `ansiX923Pad` / `ansiX923Unpad` — ANSI X9.23 (`0x00*` + length byte, constant-time verification)
- `zeroPad` / `zeroUnpad` — ZeroPadding
- `ctrIncBe128(counter: []u8)` / `ctrIncLe128(counter: []u8)` — Overflow-safe 128-bit BE/LE AES-CTR counter increment (wrapping mod 2^128)
- `xorBlock(dst: []u8, a: []const u8, b: []const u8): bool` — CBC/CTR block XOR
- `wipeBlock(buf: []u8)` — Zeroizes buffer via `@cap/subtle-ct`
