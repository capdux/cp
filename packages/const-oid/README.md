[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`const-oid`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（包名 `const-oid`）。

# const-oid

Pure CP, zero-allocation ITU-T X.690 §8.19 ASN.1 Object Identifier (OID) parser, encoder, and comparator (parity with RustCrypto `const-oid`).

## Features

- Fixed-size stack storage (`[16]u32` arcs + canonical DER buffer), zero heap allocation
- Strict dotted-decimal string parsing (`ObjectIdentifier.fromStr`) rejecting invalid root arcs (`> 2`), invalid second arcs (`arc0 < 2 && arc1 > 39`), single-arc OIDs, empty segments, leading zeros, and `u32` overflow
- Strict canonical DER decoding (`ObjectIdentifier.fromDer` / `ObjectIdentifier.fromDerTlv`) rejecting non-canonical leading `0x80` bytes and truncated Base-128 VLQ sequences
- DER value encoding (`toDer`), DER TLV encoding (`toDerTlv` with tag `0x06`), and dotted-decimal formatting (`toStr`)
- Structural comparison (`eq`, `isPrefixOf`, `arcCount`, `arcAt`)
- Standard PKI/X.509 OID constants (`OID_RSA_ENCRYPTION`, `OID_SHA256_WITH_RSA`, `OID_EC_PUBLIC_KEY`, `OID_PRIME256V1`, `OID_ED25519`, `OID_COMMON_NAME`, `OID_SHA256`, etc.)

## License

MIT
