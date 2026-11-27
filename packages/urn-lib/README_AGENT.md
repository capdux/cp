# urn-lib — AI Agent Quick Ref

Zero-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Urn,
    invalidUrn,
    urnIsSpace,
    isValidNid,
    parseUrn,
    urnEquivalent,
    pushSlice,
    pushLowerSlice,
} from "urn-lib";
```

## Core Types & Constants

```cp
class Urn { nid: string, nss: string, r_component: string, q_component: string, /* +2 fields */ }
```

## API Signatures

```cp
fn invalidUrn(): Urn;
fn urnIsSpace(b: u8): bool;
fn isValidNid(nid: string): bool;
fn parseUrn(input: string): Urn;
fn urnEquivalent(a: Urn, b: Urn): bool;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
fn formatUrn(u: Urn, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidNid } from "urn-lib";

pub fn main() {
    let result = isValidNid("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"urn-lib"`; install via `cap add urn-lib`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
