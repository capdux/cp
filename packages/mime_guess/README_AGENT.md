# mime_guess — AI Agent Quick Ref

Zero-allocation file path and extension MIME type guesser for CP (clean-room MIT, inspired by Rust mime_guess v2, powered by mime-types). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MimeGuess,
    mgsStartsWith,
    mgsHasPathSep,
    mgsIsCompressibleMime,
    extractExt,
    getExtForMime,
    getCharsetForMime,
} from "mime_guess";
```

## Core Types & Constants

```cp
class MimeGuess { mime: string, found: bool } // methods: fromExt(...) +9
```

## API Signatures

```cp
fn mgsStartsWith(s: string, prefix: string): bool;
fn mgsHasPathSep(s: string): bool;
fn mgsIsCompressibleMime(mime_type: string): bool;
fn extractExt(path: string): string?;
fn getExtForMime(mime_type: string): string?;
fn getCharsetForMime(mime_type: string): string?;
```

## Minimal Usage

```cp
import { getExtForMime } from "mime_guess";

pub fn main() {
    let result = getExtForMime("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mime_guess"`; install via `cap add mime_guess`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
