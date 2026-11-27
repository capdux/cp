# sct — AI Agent Quick Ref

Zero-allocation RFC 6962 Certificate Transparency Signed Certificate Timestamp (SCT) parser and encoder (rustls/sct.rs parity) for CP (`v1.0.0`, `Apache-2.0 OR MIT`)

## Import

```cp
import {
    SctVersion,
    HashAlgorithm,
    SignatureAlgorithm,
    SignedCertificateTimestamp,
    sctInvalid,
    sctReadBeU16,
    sctReadBeU64,
    sctWriteBeU16,
} from "sct";
```

## Core Types & Constants

```cp
enum SctVersion { V1, Unknown }
enum HashAlgorithm { None, Md5, Sha1, Sha224, Sha256, Sha384, ... }
enum SignatureAlgorithm { Anonymous, Rsa, Dsa, Ecdsa, Ed25519, Ed448, ... }
class SignedCertificateTimestamp { valid: bool, version: SctVersion, log_id: [32]u8, timestamp_ms: u64, /* +4 fields */ }
```

## API Signatures

```cp
fn sctInvalid(): SignedCertificateTimestamp;
fn sctReadBeU16(b: &[]u8, off: usize): u16;
fn sctReadBeU64(b: &[]u8, off: usize): u64;
fn sctWriteBeU16(v: u16, out: []u8, off: usize);
fn sctWriteBeU64(v: u64, out: []u8, off: usize);
fn sctDecodeHashAlg(b: u8): HashAlgorithm;
fn sctEncodeHashAlg(alg: HashAlgorithm): u8;
fn sctDecodeSigAlg(b: u8): SignatureAlgorithm;
fn sctEncodeSigAlg(alg: SignatureAlgorithm): u8;
fn parseSct(data: &[]u8): SignedCertificateTimestamp;
fn parseSctList(tls_ext: &[]u8, out: []SignedCertificateTimestamp): i64;
fn encodeSct(sct: SignedCertificateTimestamp, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseSct } from "sct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseSct(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sct"`; install via `cap add sct`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
