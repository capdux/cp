# password-hash — AI Agent Quick Ref

Zero-allocation pure CP PHC string format parser, serializer, and constant-time comparator (RustCrypto password-hash parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PhcParam,
    PhcHash,
    phchEmptyHash,
    phchIsLowerAlnumHyphen,
    phchIsParamValueChar,
    phchIsB64Char,
    phchIsValidAlgo,
    phchIsValidParamKey,
} from "password-hash";
```

## Core Types & Constants

```cp
class PhcParam { key: string, value: string }
class PhcHash { algorithm: string, version: u32, has_version: bool, params: [8]PhcParam, /* +4 fields */ }
```

## API Signatures

```cp
fn phchEmptyHash(): PhcHash;
fn phchIsLowerAlnumHyphen(c: u8): bool;
fn phchIsParamValueChar(c: u8): bool;
fn phchIsB64Char(c: u8): bool;
fn phchIsValidAlgo(s: string): bool;
fn phchIsValidParamKey(s: string): bool;
fn phchIsValidParamValue(s: string): bool;
fn phchIsValidSalt(s: string): bool;
fn phchIsValidHash(s: string): bool;
fn phchContainsByte(s: string, target: u8): bool;
fn phchParseU32(s: string): i64;
fn phchStrEq(a: string, b: string): bool;
fn phchAppendBytes(out: []u8, pos: usize, src: string): usize;
fn phchAppendU32(out: []u8, pos: usize, val: u32): usize;
fn parsePhc(phc_str: string): PhcHash;
fn formatPhc(h: PhcHash, out: []u8): usize;
fn getPhcParam(h: PhcHash, key: string): string?;
fn ctEqual(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { parsePhc } from "password-hash";

pub fn main() {
    let result = parsePhc("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"password-hash"`; install via `cap add password-hash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
