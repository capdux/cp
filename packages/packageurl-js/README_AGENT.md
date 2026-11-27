# packageurl-js — AI Agent Quick Ref

Zero-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical serializer (SBOM standard) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Purl,
    invalidPurl,
    purlIsSpace,
    isValidType,
    trimSlashes,
    parsePurl,
    getQualifier,
    pushSlice,
} from "packageurl-js";
```

## Core Types & Constants

```cp
class Purl { pkg_type: string, namespace: string, name: string, version: string, /* +3 fields */ }
```

## API Signatures

```cp
fn invalidPurl(): Purl;
fn purlIsSpace(b: u8): bool;
fn isValidType(pkg_type: string): bool;
fn trimSlashes(s: string): string;
fn parsePurl(input: string): Purl;
fn getQualifier(purl: Purl, key: string): string;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
fn formatPurl(p: Purl, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidType } from "packageurl-js";

pub fn main() {
    let result = isValidType("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"packageurl-js"`; install via `cap add packageurl-js`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
