# pkcs8@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5958 PKCS#8 PrivateKeyInfo / OneAsymmetricKey & RFC 8410 Ed25519 DER/PEM codec reusing spki, const-oid, pem-rfc7468, and subtle in pure CP. | License: MIT
> Import: `import { ... } from "pkcs8";` | Full types: `src/index.d.cp`
> Deps: `const-oid`, `pem-rfc7468`, `spki`, `subtle`

## Functions
- `export fn pk8ZeroPrivBuf(): [160]u8;`
- `export fn pk8ZeroPubBuf(): [64]u8;`
- `export fn pk8DerLenSize(n: usize): usize;`
- `export fn pk8WriteDerLen(out: []u8, pos: usize, n: usize): usize;`
- `export fn pk8ReadDerLen(der: &[]u8, pos: usize): Pk8LenDecode;`
- `export fn encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64;`
- `export fn decodePkcs8Der(der: &[]u8): Pkcs8DecodeResult;`
- `export fn encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64;`
- `export fn decodePkcs8Pem(pem: string): Pkcs8DecodeResult;`
