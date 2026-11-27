# oid-registry — AI Agent Quick Ref

Zero-allocation static X.509/PKCS/TLS/NIST/RFC OID registry with DER OID encoder and decoder (Rust oid-registry parity) for CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    OidEntry,
    OidrDecodeStep,
    oidrEntryAt,
    oidrStrEq,
    oidEntryCount,
    lookupOid,
    lookupShortName,
    oidrEncodeBase128,
} from "oid-registry";
```

## Core Types & Constants

```cp
class OidEntry { oid: string, short_name: string, description: string, category: string }
class OidrDecodeStep { val: u64, next_pos: usize, ok: bool }
```

## API Signatures

```cp
fn oidrEntryAt(idx: usize): OidEntry;
fn oidrStrEq(a: string, b: string): bool;
fn oidEntryCount(): usize;
fn lookupOid(dotted: string): OidEntry?;
fn lookupShortName(sn: string): OidEntry?;
fn oidrEncodeBase128(val: u64, out: []u8, pos: usize): usize?;
fn oidrWriteU64Dec(val: u64, out: []u8, pos: usize): usize?;
fn encodeOidDer(dotted: string, out: []u8): usize;
fn oidrDecodeBase128(der: &[]u8, start: usize): OidrDecodeStep;
fn decodeOidDer(der: &[]u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { encodeOidDer } from "oid-registry";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeOidDer("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"oid-registry"`; install via `cap add oid-registry`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
