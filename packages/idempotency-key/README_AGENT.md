# idempotency-key — AI Agent Quick Ref

Zero-allocation IETF Idempotency-Key HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    idempIsOws,
    idempTrimOws,
    idempIsVisibleAscii,
    idempIsHex,
    isValidKey,
    parseKeyHeader,
    formatKeyHeader,
    isUuidV4Like,
} from "idempotency-key";
```

## API Signatures

```cp
fn idempIsOws(c: u8): bool;
fn idempTrimOws(s: &[]u8): &[]u8;
fn idempIsVisibleAscii(c: u8): bool;
fn idempIsHex(c: u8): bool;
fn isValidKey(key: string): bool;
fn parseKeyHeader(header: string, out: []u8): usize;
fn formatKeyHeader(key: string, out: []u8): usize;
fn isUuidV4Like(key: string): bool;
fn idempMulWrap64(a: u64, b: u64): u64;
fn idempFnvUpdate(h: u64, data: &[]u8): u64;
fn idempFnvByte(h: u64, b: u8): u64;
fn fingerprintRequest(method: string, path: string, body: &[]u8): u64;
fn evaluateState(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32;
```

## Minimal Usage

```cp
import { isValidKey } from "idempotency-key";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"idempotency-key"`; install via `cap add idempotency-key`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
