[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# fast-uri

Pure CP, zero-allocation **RFC 3986 URI** parser, serializer, relative URI resolver (Section 5.2 dot-segment removal), and canonical equivalence comparator.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/fast-uri) |
| **Upstream Package** | `fast-uri` (v3 parity, Fastify ecosystem) |
| **Upstream Repo** | https://github.com/fastify/fast-uri |
| **CP Package** | `fast-uri` (`1.0.0`) |
| **License** | BSD-3-Clause |

Keep the exact same package name in CP — no renaming required:

```typescript
// TypeScript (npm)
import * as URI from "fast-uri";

// CP (cap)
import { UriComponents, parseUri, serializeUri, resolveUri, equalUri } from "fast-uri";
```

## Installation

```bash
cap add fast-uri
```

## Key Features & Performance Highlights

- **Zero Heap Allocation**: `parseUri` returns zero-copy string slices (`scheme`, `userinfo`, `host`, `path`, `query`, `fragment`) directly referencing the input string; `serializeUri` and `resolveUri` write into caller `[]u8` buffers.
- **Full RFC 3986 Compliance**: Supports IPv6 bracketed hosts (`[::1]:8080`), empty query/fragment distinction (`has_query` / `has_fragment`), strict port range validation (`0..65535`), and Section 5.2.4 dot-segment normalization (`remove_dot_segments`).
- **Canonical Normalization & Equivalence**: `equalUri(a, b)` lowercases scheme/host, uppercases percent-encoding hex digits, removes default ports (`http:80`, `https:443`, `ws:80`, `wss:443`, `ftp:21`), and collapses dot-segments before comparing.

## Quick Start

### Parse, Serialize, Resolve & Compare

```typescript
import { parseUri, serializeUri, resolveUri, equalUri } from "fast-uri";

fn main(): void {
    const uri = parseUri("https://user@Example.COM:443/a/b/../c?k=1#sec");
    if (!uri.valid) {
        return;
    }
    // uri.scheme == "https", uri.host == "Example.COM", uri.port == 443, uri.path == "/a/b/../c"

    // Serialize back to a buffer
    let ser_buf: [256]u8 = undefined;
    const ser_len = serializeUri(uri, ser_buf[0..]);
    _ = ser_buf[0..ser_len];

    // RFC 3986 relative reference resolution
    let res_buf: [256]u8 = undefined;
    const res_len = resolveUri("https://example.com/api/v1/users", "../items?limit=10", res_buf[0..]);
    // res_buf[0..res_len] == "https://example.com/api/items?limit=10"
    _ = res_len;

    // Canonical equivalence check
    const same = equalUri("HTTP://Example.COM:80/a/./b", "http://example.com/a/b");
    // same == true
    _ = same;
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `UriComponents` | `{ valid, scheme, userinfo, host, port, path, query, has_query, fragment, has_fragment }` | Parsed URI components (`port == -1` when omitted) |
| `UriComponents.invalid` / `empty` | `() -> UriComponents` | Constructors for invalid or empty URI structs |
| `parseUri` | `(input: string) -> UriComponents` | Zero-copy RFC 3986 URI parser |
| `serializeUri` | `(c: UriComponents, out: []u8) -> usize` | Serialize `UriComponents` into `out` |
| `resolveUri` | `(base: string, rel: string, out: []u8) -> usize` | Resolve relative URI against `base` with dot-segment removal |
| `equalUri` | `(a: string, b: string) -> bool` | RFC 3986 normalized equivalence comparison |
| `furiRemoveDotSegments` | `(input: string, out: []u8) -> usize` | RFC 3986 Section 5.2.4 path dot-segment removal |
| `furiDefaultPort` | `(scheme: string) -> i32` | Default port for `http`/`https`/`ws`/`wss`/`ftp` (or `-1`) |

## License

BSD-3-Clause
