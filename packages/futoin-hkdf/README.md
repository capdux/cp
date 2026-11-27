[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# futoin-hkdf

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:futoin-hkdf` ([https://github.com/futoin/util-js-hkdf](https://github.com/futoin/util-js-hkdf))（原库名 `futoin-hkdf` → CP 包名 `futoin-hkdf`）

Pure CP, zero-allocation RFC 5869 HKDF-SHA256 key derivation function (parity with RustCrypto `hkdf`).

Reuses `hmacSha256` and streaming SHA-256 primitives from `cookie-signature`.

## API

- `hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool` — RFC 5869 Section 2.2 (empty salt defaults to 32 zero bytes)
- `hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool` — RFC 5869 Section 2.3 (`0 <= L <= 8160`)
- `hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool` — One-shot Extract + Expand

## License

MIT
