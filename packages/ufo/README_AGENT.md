# ufo — AI Agent Quick Ref

Zero-heap URL protocol detection, slash normalization, and safe URL joining utilities (`v1.5.4`, `MIT`)

## Import

```cp
import {
    ufoIsAlpha,
    ufoIsSchemeChar,
    ufoProtocolEnd,
    ufoCopyBytes,
    hasProtocol,
    isRelative,
    withoutLeadingSlash,
    withoutTrailingSlash,
} from "ufo";
```

## API Signatures

```cp
fn ufoIsAlpha(c: u8): bool;
fn ufoIsSchemeChar(c: u8): bool;
fn ufoProtocolEnd(url: string): usize;
fn ufoCopyBytes(src: string, out: []u8, offset: usize): usize;
fn hasProtocol(url: string): bool;
fn isRelative(url: string): bool;
fn withoutLeadingSlash(s: string): string;
fn withoutTrailingSlash(s: string): string;
fn withLeadingSlash(s: string, out: []u8): usize;
fn withTrailingSlash(s: string, out: []u8): usize;
fn cleanDoubleSlashes(url: string, out: []u8): usize;
fn joinUrl(base: string, segment: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isRelative } from "ufo";

pub fn main() {
    let result = isRelative("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ufo"`; install via `cap add ufo`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
