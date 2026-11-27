# serde_qs — AI Agent Quick Ref

Zero-allocation nested bracket query string parser & serializer (crates.io serde_qs v0.13 parity) composing percent-encoding for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    QsPair,
    QsPath,
    sqsKeyEncodeSet,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
} from "serde_qs";
```

## Core Types & Constants

```cp
class QsPair { key: string, value: string }
class QsPath { segments: [8]string, depth: usize }
```

## API Signatures

```cp
fn sqsKeyEncodeSet(): AsciiSet;
fn parseKeyPath(key: string): QsPath;
fn parseQs(query: string, out_pairs: []QsPair): usize;
fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize?;
fn serializeQs(pairs: &[]QsPair, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseKeyPath } from "serde_qs";

pub fn main() {
    let result = parseKeyPath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"serde_qs"`; install via `cap add serde_qs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
