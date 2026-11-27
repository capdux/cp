# sec1@1.0.0 (Agent Quickref)

> Pure CP zero-allocation SEC1 v2.0 elliptic curve point encoding & RFC 5915 ECPrivateKey DER/PEM codec (reuses const-oid, pem-rfc7468, and subtle) | License: MIT
> Import: `import { ... } from "sec1";` | Full types: `src/index.d.cp`
> Deps: `const-oid`, `pem-rfc7468`, `subtle`

## Functions
- `export fn makeEcPrivateKey(priv_bytes: &[]u8, has_curve: bool, curve_oid: ObjectIdentifier, has_pub_key: bool, pub_key: EncodedPoint): EcPrivateKeyResult;`
- `export fn encodeEcPrivateKeyDer(key: EcPrivateKey, out: []u8): i64;`
- `export fn decodeEcPrivateKeyDer(der: &[]u8): EcPrivateKeyResult;`
- `export fn encodeEcPrivateKeyPem(key: EcPrivateKey, crlf: bool, out: []u8): i64;`
- `export fn decodeEcPrivateKeyPem(pem: string): EcPrivateKeyResult;`
