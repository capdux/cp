# jsonpointer — AI Agent Quick Ref

Zero-allocation RFC 6901 JSON Pointer validator, escaper, segment navigator, array index parser, and iterator in pure CP (`v0.1.0`, `MIT`)

## Import

```cp
import {
    PointerIter,
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
} from "jsonpointer";
```

## Core Types & Constants

```cp
class PointerIter { ptr: string, pos: usize, valid: bool } // methods: init(...) +1
```

## API Signatures

```cp
fn isValid(ptr: string): bool;
fn escapeToken(token: string, out: []u8): usize;
fn unescapeToken(token: string, out: []u8): usize;
fn countSegments(ptr: string): usize;
fn hasSegment(ptr: string, index: usize): bool;
fn getSegment(ptr: string, index: usize, out: []u8): usize;
fn parseArrayIndex(token: string): i64;
fn appendSegment(base: string, token: string, out: []u8): usize;
fn parentPointer(ptr: string): string;
```

## Minimal Usage

```cp
import { isValid } from "jsonpointer";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsonpointer"`; install via `cap add jsonpointer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
