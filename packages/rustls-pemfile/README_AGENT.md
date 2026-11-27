# rustls-pemfile@1.0.0 (Agent Quickref)

> Zero-allocation streaming TLS PEM bundle reader for X.509 certificates, PKCS#8/PKCS#1/SEC1 private keys, SPKI public keys, CRLs, and CSRs reusing @cap/pem-rfc7468 (rustls-pemfile v2 parity) in pure CP. | License: MIT
> Import: `import { ... } from "rustls-pemfile";` | Full types: `src/index.d.cp`
> Deps: `pem-rfc7468`

## Functions
- `export fn rpfStrEq(a: string, b: string): bool;`
- `export fn isPrivateKeyKind(kind: u8): bool;`
- `export fn classifyPemLabel(label: string): u8;`
- `export fn readFirstPrivateKey(input: string, der_out: []u8): PemItem;`
