[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ecdsa

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ecdsa` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures))（原库名 `ecdsa` → CP 包名 `ecdsa`）

Pure CP, zero-allocation ECDSA signature codec supporting RFC 3279 §2.2.3 ASN.1 DER (`SEQUENCE { r INTEGER, s INTEGER }`) and fixed-width `r || s` for NIST P-256 (64B) and P-384 (96B), with BIP-62 Low-S malleability normalization (parity with RustCrypto `ecdsa` / `signature`).

Reuses `der` for strict ASN.1 DER encoding/decoding and `subtle` for constant-time signature comparison.

## API

- `EcdsaSignature.fromFixed(bytes: []const u8): EcdsaSigResult` (64B P-256 / 96B P-384)
- `EcdsaSignature.fromDer(der: []const u8, coord_len: usize): EcdsaSigResult`
- `toFixed(out: []u8): usize`
- `toDer(out: []u8): i64`
- `isLowSP256(): bool` / `normalizeLowSP256(*_): bool`
- `ctEq(other: EcdsaSignature): bool`

## License

MIT
