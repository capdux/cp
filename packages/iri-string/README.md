[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# iri-string

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`iri-string`](https://github.com/lo48576/iri-string) (`v0.7` / `MIT`), CP package `iri-string@1.0.0`.

Zero-allocation RFC 3986 / RFC 3987 URI & IRI component parser, dot-segment normalizer, and relative reference resolver in pure CP.

## Why CP `iri-string`?

- **Zero Heap Allocation**: `parseIri` returns byte-offset spans (`IriParts`) into the original input string with zero copies; `removeDotSegments` and `resolveReference` write directly into caller-supplied `[]u8` buffers.
- **Zero-Overhead Stack Scratch**: Internal path and merge buffers (`[512]u8`) inside `resolveReference` use `undefined` initialization, avoiding 1KB of redundant stack zeroing per call.
- **RFC 3986 §5.2 Compliant**: Implements full `remove_dot_segments` (`.`, `..`, leading/trailing slashes) and `resolveReference(base, rel, out)` across scheme, authority, path, query, and fragment cases.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`iri-string`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use iri_string::types::{IriStr, IriReferenceStr};
let resolved = rel.resolve_against(base);
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    IriParts, parseIri, removeDotSegments, resolveReference
} from "iri-string";
```

## Quick Start

### Parse URI / IRI Components (Zero-Copy)

```typescript
import { parseIri } from "iri-string";

export fn inspectUrl(url: string): bool {
    let p = parseIri(url);
    if (!p.valid || !p.has_scheme) {
        return false;
    }
    let scheme = url[p.scheme_start..(p.scheme_start + p.scheme_len)];
    let path = url[p.path_start..(p.path_start + p.path_len)];
    _ = scheme;
    _ = path;
    return p.has_auth;
}
```

### Resolve Relative References & Normalize Dot Segments

```typescript
import { resolveReference, removeDotSegments } from "iri-string";

export fn demoResolve(): usize {
    let mut out: [512]u8 = undefined;
    // RFC 3986 §5.4 example: "http://a/b/c/d;p?q" + "../g" -> "http://a/b/g"
    let n1 = resolveReference("http://a/b/c/d;p?q", "../g", out[0..512]);

    let mut clean: [256]u8 = undefined;
    // "/a/b/c/./../../g" -> "/a/g"
    let n2 = removeDotSegments("/a/b/c/./../../g", clean[0..256]);
    return n1 + n2;
}
```

## API Reference

- **`IriParts`**: Offset/length spans (`scheme_start/len`, `auth_start/len`, `path_start/len`, `query_start/len`, `frag_start/len`) and flags (`has_scheme`, `has_auth`, `has_query`, `has_frag`, `valid`).
- **`parseIri(s: string): IriParts`** — Zero-copy RFC 3986/3987 component parser.
- **`removeDotSegments(path: string, out: []u8): usize`** — RFC 3986 §5.2.4 path normalization; returns bytes written.
- **`resolveReference(base: string, rel: string, out: []u8): usize`** — RFC 3986 §5.2.2 relative reference resolution against `base`; returns bytes written (`0` on invalid input or buffer overflow).

## License

MIT
