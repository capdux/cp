# oci-distribution — AI Agent Quick Ref

Zero-allocation OCI / Docker image reference parser, validator, and normalizer for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ImageRef,
    drIsLower,
    drIsUpper,
    drIsDigit,
    drIsLowerHex,
    drEqBytes,
    drContainsByte,
    isValidTag,
} from "oci-distribution";
```

## Core Types & Constants

```cp
class ImageRef { registry: string, repository: string, tag: string, digest: string, /* +1 fields */ }
```

## API Signatures

```cp
fn drIsLower(c: u8): bool;
fn drIsUpper(c: u8): bool;
fn drIsDigit(c: u8): bool;
fn drIsLowerHex(c: u8): bool;
fn drEqBytes(a: &[]u8, b: &[]u8): bool;
fn drContainsByte(s: &[]u8, target: u8): bool;
fn isValidTag(tag: string): bool;
fn isValidDigest(digest: string): bool;
fn drIsValidRegistry(reg: &[]u8): bool;
fn drIsValidRepository(repo: &[]u8): bool;
fn parseImageRef(input: string): ImageRef;
fn drWrite(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn formatNormalized(ref_in: ImageRef, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidTag } from "oci-distribution";

pub fn main() {
    let result = isValidTag("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"oci-distribution"`; install via `cap add oci-distribution`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
