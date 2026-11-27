# sct@1.0.0 (Agent Quickref)

> Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder (rustls/sct.rs parity) for CP | License: Apache-2.0 OR MIT
> Import: `import { ... } from "sct";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SctVersion`
- `export enum HashAlgorithm`
- `export enum SignatureAlgorithm`

## Functions
- `export fn sctInvalid(): SignedCertificateTimestamp;`
- `export fn sctReadBeU16(b: &[]u8, off: usize): u16;`
- `export fn sctReadBeU64(b: &[]u8, off: usize): u64;`
- `export fn sctWriteBeU16(v: u16, out: []u8, off: usize);`
- `export fn sctWriteBeU64(v: u64, out: []u8, off: usize);`
- `export fn sctDecodeHashAlg(b: u8): HashAlgorithm;`
- `export fn sctEncodeHashAlg(alg: HashAlgorithm): u8;`
- `export fn sctDecodeSigAlg(b: u8): SignatureAlgorithm;`
- `export fn sctEncodeSigAlg(alg: SignatureAlgorithm): u8;`
- `export fn parseSct(data: &[]u8): SignedCertificateTimestamp;`
- `export fn parseSctList(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64;`
- `export fn encodeSct(sct: SignedCertificateTimestamp, out: []u8): usize;`
