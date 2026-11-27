[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `pkcs8`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pkcs8` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Zero-allocation RFC 5958 PKCS#8 `PrivateKeyInfo` (v1) / `OneAsymmetricKey` (v2) & RFC 8410 Ed25519 DER/PEM codec in pure CP (RustCrypto `pkcs8` parity).

## Reused Ecosystem Packages

- `spki` — `AlgorithmIdentifier`, `encodeAlgorithmId`, `decodeAlgorithmId`
- `const-oid` — `ObjectIdentifier` & standard PKI OIDs (`OID_ED25519`, `OID_EC_PUBLIC_KEY`, `OID_PRIME256V1`)
- `pem-rfc7468` — RFC 7468 `PRIVATE KEY` PEM encoding & strict decoding
- `subtle` — `zeroizeBytes` for secure private key buffer wiping

## API

- `PrivateKeyInfo` — `empty()`, `ed25519(seed32)`, `ed25519V2(seed32, pub32)`, `p256(ec_priv_der)`, `zeroize(*_)`
- `encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64`
- `decodePkcs8Der(der: []const u8): Pkcs8DecodeResult`
- `encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64`
- `decodePkcs8Pem(pem: string): Pkcs8DecodeResult`
