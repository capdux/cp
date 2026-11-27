# etag — AI Agent Quick Ref

Zero-allocation RFC 9110 HTTP ETag generator, parser, and If-None-Match / If-Match evaluator in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EntityTag,
    etgIsSpace,
    etgIsEtagChar,
    etgStrEq,
    parseEtag,
    formatEtag,
    strongEqual,
    weakEqual,
} from "etag";
```

## Core Types & Constants

```cp
class EntityTag { weak: bool, tag: string }
```

## API Signatures

```cp
fn etgIsSpace(b: u8): bool;
fn etgIsEtagChar(b: u8): bool;
fn etgStrEq(a: string, b: string): bool;
fn parseEtag(input: string, out: &mut EntityTag): bool;
fn formatEtag(weak: bool, tag: string, out: []u8): usize;
fn strongEqual(a: EntityTag, b: EntityTag): bool;
fn weakEqual(a: EntityTag, b: EntityTag): bool;
fn etgHexLower(nibble: u8): u8;
fn etgWriteHexU64(val: u64, out: []u8, start: usize): usize;
fn etgHexLenU64(val: u64): usize;
fn generateStatEtag(size: u64, mtime_ms: u64, out: []u8): usize;
fn etgFnv1a64(data: &[]u8): u64;
fn generateContentEtag(data: &[]u8, weak: bool, out: []u8): usize;
fn etgMatchHeaderList(header: string, current: EntityTag, require_strong: bool): bool;
fn matchIfNoneMatch(header: string, current: EntityTag): bool;
fn matchIfMatch(header: string, current: EntityTag): bool;
fn compareStrong(a: string, b: string): bool;
fn compareWeak(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { parseEtag } from "etag";

pub fn main() {
    let result = parseEtag("example", /* out: &mut EntityTag */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"etag"`; install via `cap add etag`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
