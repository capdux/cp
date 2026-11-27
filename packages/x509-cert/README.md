[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# x509-cert

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:x509-cert` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Pure CP, zero-allocation RFC 5280 X.509 v1/v2/v3 Certificate DER & PEM parser and encoder (parity with RustCrypto `x509-cert`).

Composes 4 official CP PKI packages with zero code duplication:
- `der` for strict ITU-T X.690 DER reading/writing
- `const-oid` for DN attribute OIDs (`CN`, `O`, `C`)
- `spki` for `AlgorithmIdentifier` & `SubjectPublicKeyInfo`
- `pem-rfc7468` for `CERTIFICATE` PEM encoding/decoding

## API

- `X509Validity`: `fromStrs(not_before, not_after)` (UTCTime `0x17` & GeneralizedTime `0x18`)
- `X509Name`: `init(cn, org, country)`, `eq(other)`
- `X509Certificate`: `version` (1/2/3), `serial`, `signature_alg`, `issuer`, `validity`, `subject`, `spki`, `signature`
- `encodeX509Der(cert, out): i64` / `decodeX509Der(der): X509DecodeResult`
- `encodeX509Pem(cert, crlf, out): i64` / `decodeX509Pem(pem): X509DecodeResult`
- `isSelfSigned(cert): bool`

## License

MIT
