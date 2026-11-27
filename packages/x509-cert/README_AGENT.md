# x509-cert@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 5280 X.509 v1/v2/v3 certificate DER & PEM codec (reuses der, const-oid, spki, and pem-rfc7468) | License: MIT
> Import: `import { ... } from "x509-cert";` | Full types: `src/index.d.cp`
> Deps: `const-oid`, `der`, `pem-rfc7468`, `spki`

## Functions
- `export fn encodeX509Der(cert: X509Certificate, out: []u8): i64;`
- `export fn decodeX509Der(der: &[]u8): X509DecodeResult;`
- `export fn encodeX509Pem(cert: X509Certificate, crlf: bool, out: []u8): i64;`
- `export fn decodeX509Pem(pem: string): X509DecodeResult;`
- `export fn isSelfSigned(cert: X509Certificate): bool;`
