# accept-header — AI Agent Quick Ref

Zero-heap RFC 7231 HTTP Accept header parser and media-type negotiator with quality-factor & specificity ranking (clean-room port of Rust accept-header) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AcceptEntry,
    AcceptHeader,
    acphFindSlash,
    acphIsStar,
    acphMediaSpecificity,
    acphMatchesRange,
    acphEmptyEntry,
    acphEmptySpec,
} from "accept-header";
```

## Core Types & Constants

```cp
class AcceptEntry { media_range: string, quality_milli: u16, specificity: u8 }
class AcceptHeader { entries: [16]AcceptEntry, count: usize } // methods: parse(...) +3
```

## API Signatures

```cp
fn acphFindSlash(s: string): usize;
fn acphIsStar(s: string): bool;
fn acphMediaSpecificity(range: string): u8;
fn acphMatchesRange(range: string, mime: string): bool;
fn acphEmptyEntry(): AcceptEntry;
fn acphEmptySpec(): QualitySpec;
fn negotiateMediaType(header: string, available: []string): string?;
```

## Minimal Usage

```cp
import { acphFindSlash } from "accept-header";

pub fn main() {
    let result = acphFindSlash("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"accept-header"`; install via `cap add accept-header`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
