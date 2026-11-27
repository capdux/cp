[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http-digest-client

Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication challenge & authorization parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:http-digest-client` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:digest_auth` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `http-digest-client` → CP 包名 `http-digest-client`）

| Metadata | Value |
| --- | --- |
| Package | `http-digest-client` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http-digest-client
```

```cp
import {
    DigestChallenge,
    DigestResponse,
    digIsSpace,
    trimSlice,
    stripDigestPrefix,
    unquoteVal,
    parseChallenge,
    parseAuthorization,
} from "http-digest-client";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DigestChallenge` | `class` | Fields: `realm: string, nonce: string, opaque_val: string, qop: string, algorithm: string, stale: bool, +1 more` |
| `DigestResponse` | `class` | Fields: `username: string, realm: string, nonce: string, uri: string, qop: string, nc: string, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `digIsSpace` | `(b: u8): bool` | Invokes `digIsSpace` and returns `bool`. |
| `trimSlice` | `(s: string): string` | Invokes `trimSlice` and returns `string`. |
| `stripDigestPrefix` | `(header: string): string` | Invokes `stripDigestPrefix` and returns `string`. |
| `unquoteVal` | `(v: string): string` | Invokes `unquoteVal` and returns `string`. |
| `parseChallenge` | `(header: string): DigestChallenge` | Parses input via `parseChallenge` and returns `DigestChallenge`. |
| `parseAuthorization` | `(header: string): DigestResponse` | Parses input via `parseAuthorization` and returns `DigestResponse`. |
| `supportsQop` | `(qop_list: string, target: string): bool` | Invokes `supportsQop` and returns `bool`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushQuotedField` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | Mutates or appends state via `pushQuotedField`. |
| `pushTokenField` | `(dst: []u8, pos: usize, first: bool, key: string, val: string): i64` | Mutates or appends state via `pushTokenField`. |
| `formatAuthorization` | `(resp: DigestResponse, out: []u8): usize` | Serializes/formats data via `formatAuthorization` returning `usize`. |
| `formatChallenge` | `(ch: DigestChallenge, out: []u8): usize` | Serializes/formats data via `formatChallenge` returning `usize`. |

## Minimal Example

```cp
import { parseChallenge } from "http-digest-client";

pub fn main() {
    let result = parseChallenge("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
