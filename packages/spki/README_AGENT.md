# spki@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 5280 SubjectPublicKeyInfo (SPKI) & AlgorithmIdentifier DER/PEM codec (reuses const-oid and pem-rfc7468) | License: MIT
> Import: `import { ... } from "spki";` | Full types: `src/index.d.cp`
> Deps: `const-oid`, `pem-rfc7468`

## Functions
- `export fn encodeAlgorithmId(alg: AlgorithmIdentifier, out: []u8): i64;`
- `export fn decodeAlgorithmId(der: &[]u8): AlgIdDecodeResult;`
- `export fn encodeSpkiDer(spki: SubjectPublicKeyInfo, out: []u8): i64;`
- `export fn decodeSpkiDer(der: &[]u8): SpkiDecodeResult;`
- `export fn encodeSpkiPem(spki: SubjectPublicKeyInfo, crlf: bool, out: []u8): i64;`
- `export fn decodeSpkiPem(pem: string): SpkiDecodeResult;`
- `export fn fingerprintSpki(der: &[]u8): u64;`
