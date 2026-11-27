# `@cap/pkcs1`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pkcs1` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

Zero-allocation RFC 8017 Appendix A.1.1 (`RSAPublicKey`) and Appendix A.1.2 (`RSAPrivateKey`) DER/PEM codec in pure CP (RustCrypto `pkcs1` parity).

## Reused Ecosystem Packages

- `@cap/der-asn1` — `DerReader`, `DerWriter`, `TAG_INTEGER` for strict X.690 DER TLV/SEQUENCE/INTEGER encoding and decoding
- `@cap/pem-rfc7468` — `encodePem`, `decodePem` for `RSA PUBLIC KEY` and `RSA PRIVATE KEY` PEM blocks
- `@cap/subtle-ct` — `zeroizeBytes` for secure private key buffer wiping

## API

- `RsaPublicKey` (`n: [512]u8`, `n_len: usize`, `e: u64`) — supports up to 4096-bit RSA keys
- `RsaPrivateKey` (`version`, `n`, `e`, `d`, `p`, `q`, `dp`, `dq`, `qinv`, `publicKey()`, `zeroize(*_)`)
- `modulusBitLen(pub_key: RsaPublicKey): usize`
- `encodeRsaPublicKeyDer` / `decodeRsaPublicKeyDer` / `encodeRsaPublicKeyPem` / `decodeRsaPublicKeyPem`
- `encodeRsaPrivateKeyDer` / `decodeRsaPrivateKeyDer` / `encodeRsaPrivateKeyPem` / `decodeRsaPrivateKeyPem`
