# http — AI Agent Quick Ref

Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types (crates.io http v1 parity) composing unicase for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StatusCode,
    htpStrEq,
    htpIsTchar,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    parseHttpVersion,
} from "http";
```

## Core Types & Constants

```cp
class StatusCode { code: u16 } // methods: fromU16(...) +7
```

## API Signatures

```cp
fn htpStrEq(a: string, b: string): bool;
fn htpIsTchar(b: u8): bool;
fn parseMethod(s: string): u8;
fn methodAsStr(m: u8): string;
fn isSafeMethod(m: u8): bool;
fn isIdempotentMethod(m: u8): bool;
fn parseHttpVersion(s: string): u8;
fn httpVersionAsStr(v: u8): string;
fn isValidHeaderName(name: string): bool;
fn headerNameEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { parseMethod } from "http";

pub fn main() {
    let result = parseMethod("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"http"`; install via `cap add http`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
