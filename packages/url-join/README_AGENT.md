# url-join — AI Agent Quick Ref

Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities (url-join / ufo parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PartSplit,
    pushByte,
    isSchemeChar,
    schemePrefixLen,
    splitPart,
    trimLeadingSlashes,
    trimTrailingSlashes,
    urlJoin,
} from "url-join";
```

## Core Types & Constants

```cp
class PartSplit { path: string, query: string, hash: string }
```

## API Signatures

```cp
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn isSchemeChar(c: u8): bool;
fn schemePrefixLen(s: string): usize;
fn splitPart(p: string): PartSplit;
fn trimLeadingSlashes(s: string): string;
fn trimTrailingSlashes(s: string): string;
fn urlJoin(parts: []string, out: []u8): string;
fn normalizeUrlPath(path: string, out: []u8): string;
fn withQuery(base_url: string, key: string, value: string, out: []u8): string;
fn findQueryOrHash(url: string): usize;
fn withTrailingSlash(url: string, out: []u8): string;
fn withoutTrailingSlash(url: string, out: []u8): string;
```

## Minimal Usage

```cp
import { schemePrefixLen } from "url-join";

pub fn main() {
    let result = schemePrefixLen("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"url-join"`; install via `cap add url-join`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
