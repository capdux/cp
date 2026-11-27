[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/sec1

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sec1` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Pure CP, zero-allocation SEC1 v2.0 elliptic curve point encoding and RFC 5915 `ECPrivateKey` DER/PEM codec (parity with RustCrypto `sec1`).

Reuses `@cap/const-oid` for curve OIDs, `@cap/pem-rfc7468` for `EC PRIVATE KEY` PEM encoding/decoding, and `@cap/subtle-ct` for secure private key zeroization.

## API

- `EncodedPoint` (P-256 / P-384 / P-521): `identity()`, `fromUncompressed(x, y)`, `fromCompressed(x, y_is_odd)`, `fromBytes(bytes)`, `fromBytesForCoord(bytes, coord_size)`, `tag()`, `isIdentity()`, `isCompressed()`, `xBytes(out)`, `yBytes(out)`, `compress()`, `eq(other)`
- `EcPrivateKey` (RFC 5915): `makeEcPrivateKey(...)`, `encodeEcPrivateKeyDer`, `decodeEcPrivateKeyDer`, `encodeEcPrivateKeyPem`, `decodeEcPrivateKeyPem`, `zeroize(*_)`

## License

MIT
