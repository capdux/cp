[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ed25519

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ed25519` ([https://github.com/RustCrypto/signatures](https://github.com/RustCrypto/signatures)) — `Apache-2.0 OR MIT`

Zero-heap RFC 8032 Ed25519 signature and key types, canonical scalar validation ($S < L$), RFC 8410 SPKI / PKCS#8 DER encoding (OID `1.3.101.112`), and hex conversion in pure CP.

## Features

- Constants: `SIGNATURE_LENGTH = 64`, `KEY_LENGTH = 32`, `SPKI_DER_LENGTH = 44`, `PKCS8_DER_LENGTH = 48`
- `Ed25519Signature`: `fromBytes`, `fromSlice`, `fromComponents(r, s)`, `rBytes`, `sBytes`, `toBytes`, `isCanonicalS` ($S < L$ malleability protection), `fromHex`, `toHex`, `eql`
- `Ed25519PublicKey`: `fromBytes`, `fromSlice`, `toBytes`, `toSpkiDer`, `fromSpkiDer`, `fromHex`, `toHex`, `eql`
- `Ed25519SecretKey`: `fromBytes`, `fromSlice`, `toBytes`, `toPkcs8Der`, `fromPkcs8Der`, `fromHex`, `toHex`, `eql`

## License

Apache-2.0 OR MIT
