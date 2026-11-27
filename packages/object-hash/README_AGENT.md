# object-hash — AI Agent Quick Ref

Zero-allocation deterministic order-independent structured object hasher using FNV-1a 64-bit (npm object-hash parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HashField,
    ohshFnvOffset,
    ohshFnvPrime,
    ohshFeedByte,
    ohshFeedBytes,
    ohshCmpStr,
    ohshFieldLess,
    ohshHexDigit,
} from "object-hash";
```

## Core Types & Constants

```cp
class HashField { key: string, value: string, kind: u8 }
```

## API Signatures

```cp
fn ohshFnvOffset(): u64;
fn ohshFnvPrime(): u64;
fn ohshFeedByte(h: u64, b: u8): u64;
fn ohshFeedBytes(h: u64, data: &[]u8): u64;
fn ohshCmpStr(a: string, b: string): i32;
fn ohshFieldLess(a: HashField, b: HashField): bool;
fn ohshHexDigit(nibble: u8): u8;
fn hashBytesFnv64(data: &[]u8): u64;
fn hashStringFnv64(s: string): u64;
fn hashFieldsFnv64(fields: &[]HashField): u64;
fn hashFieldsHex(fields: &[]HashField, out: []u8): usize;
```

## Minimal Usage

```cp
import { hashStringFnv64 } from "object-hash";

pub fn main() {
    let result = hashStringFnv64("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"object-hash"`; install via `cap add object-hash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
