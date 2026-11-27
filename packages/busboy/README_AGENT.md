# busboy — AI Agent Quick Ref

Zero-heap RFC 7578 multipart/form-data boundary extractor, part iterator, and Content-Disposition field/filename parser. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MultipartPart,
    isWs,
    isBchar,
    isValidBoundary,
    extractBoundary,
    matchBoundaryAt,
    extractDispositionParam,
    nextPart,
} from "busboy";
```

## Core Types & Constants

```cp
class MultipartPart { headers: string, body: string, name: string, filename: string, /* +3 fields */ }
```

## API Signatures

```cp
fn isWs(c: u8): bool;
fn isBchar(c: u8): bool;
fn isValidBoundary(boundary: string): bool;
fn extractBoundary(content_type: string): string;
fn matchBoundaryAt(payload: string, pos: usize, boundary: string): bool;
fn extractDispositionParam(disp: string, key: string): string;
fn nextPart(payload: string, boundary: string, start: usize): MultipartPart;
fn countParts(payload: string, boundary: string): usize;
```

## Minimal Usage

```cp
import { isValidBoundary } from "busboy";

pub fn main() {
    let result = isValidBoundary("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"busboy"`; install via `cap add busboy`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
