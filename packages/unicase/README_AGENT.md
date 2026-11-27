# unicase — AI Agent Quick Ref

Zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing (clean-room port of Rust unicase) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Ascii,
    UniCase,
    ucDecodeUtf8,
    ucEncodeUtf8,
    foldAsciiByte,
    foldCodepoint,
    caseFold,
    eqAscii,
} from "unicase";
```

## Core Types & Constants

```cp
class Ascii { raw: string } // methods: new(...) +4
class UniCase { raw: string } // methods: new(...) +5
```

## API Signatures

```cp
fn ucDecodeUtf8(s: string, pos: usize, step_out: []usize): u32;
fn ucEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
fn foldAsciiByte(b: u8): u8;
fn foldCodepoint(cp: u32, folded_out: []u32): usize;
fn caseFold(src: string, out: []u8): usize;
fn eqAscii(a: string, b: string): bool;
fn cmpAscii(a: string, b: string): i32;
fn cmpUnicode(a: string, b: string): i32;
fn eqUnicode(a: string, b: string): bool;
fn hashAscii(s: string): u64;
fn hashUnicode(s: string): u64;
fn startsWithIgnoreCase(haystack: string, prefix: string): bool;
fn endsWithIgnoreCase(haystack: string, suffix: string): bool;
fn containsIgnoreCase(haystack: string, needle: string): bool;
```

## Minimal Usage

```cp
import { hashAscii } from "unicase";

pub fn main() {
    let result = hashAscii("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicase"`; install via `cap add unicase`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
