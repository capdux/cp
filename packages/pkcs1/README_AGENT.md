# pkcs1@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8017 PKCS#1 RSAPublicKey and RSAPrivateKey DER/PEM codec reusing @cap/der-asn1, @cap/pem-rfc7468, and @cap/subtle-ct in pure CP. | License: MIT
> Import: `import { ... } from "pkcs1";` | Full types: `src/index.d.cp`
> Deps: `der`, `pem-rfc7468`, `subtle`

## Functions
- `export fn pk1WriteBigUint(w: &mut DerWriter, raw: &[]u8): bool;`
- `export fn pk1ReadBigUint(r: &mut DerReader, dst: []u8, len_out: &mut usize): i64;`
- `export fn modulusBitLen(pub_key: RsaPublicKey): usize;`
- `export fn encodeRsaPublicKeyDer(pub_key: RsaPublicKey, out: []u8): i64;`
- `export fn decodeRsaPublicKeyDer(der: &[]u8): RsaPublicKeyResult;`
- `export fn encodeRsaPublicKeyPem(pub_key: RsaPublicKey, crlf: bool, out: []u8): i64;`
- `export fn decodeRsaPublicKeyPem(pem: string): RsaPublicKeyResult;`
- `export fn encodeRsaPrivateKeyDer(priv_key: RsaPrivateKey, out: []u8): i64;`
- `export fn decodeRsaPrivateKeyDer(der: &[]u8): RsaPrivateKeyResult;`
- `export fn encodeRsaPrivateKeyPem(priv_key: RsaPrivateKey, crlf: bool, out: []u8): i64;`
- `export fn decodeRsaPrivateKeyPem(pem: string): RsaPrivateKeyResult;`
