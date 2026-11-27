[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/spki

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:spki` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Pure CP, zero-allocation RFC 5280 `SubjectPublicKeyInfo` (SPKI) and `AlgorithmIdentifier` DER/PEM codec (parity with RustCrypto `spki`).

Reuses `@cap/const-oid` for ITU-T X.690 OID encoding/decoding and `@cap/pem-rfc7468` for RFC 7468 `PUBLIC KEY` PEM encoding/decoding.

## API

- `AlgorithmIdentifier`: `fromOid`, `withNull`, `withParamOid`, `eq`
- `SubjectPublicKeyInfo`: `ed25519(pub_key)`, `p256(pub_key)`, `rsa(pub_key_der)`
- `encodeAlgorithmId(alg, out): i64` / `decodeAlgorithmId(der): AlgIdDecodeResult`
- `encodeSpkiDer(spki, out): i64` / `decodeSpkiDer(der): SpkiDecodeResult`
- `encodeSpkiPem(spki, crlf, out): i64` / `decodeSpkiPem(pem): SpkiDecodeResult`
- `fingerprintSpki(der): u64`

## License

MIT
