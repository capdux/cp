[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @atproto/syntax

Zero-allocation AT Protocol / Bluesky at:// URI, NSID, and Record Key (rkey) validator, parser, and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@atproto/syntax` ([https://github.com/bluesky-social/atproto](https://github.com/bluesky-social/atproto)) / `crates.io:atrium-api` ([https://github.com/sugyan/atrium](https://github.com/sugyan/atrium))（原库名 `@atproto/syntax` → CP 包名 `@atproto/syntax`）

| Metadata | Value |
| --- | --- |
| Package | `@atproto/syntax` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @atproto/syntax
```

```cp
import {
    AtUri,
    invalidAtUri,
    isLowerAlpha,
    isAlpha,
    isDigit,
    isAlphaNum,
    isValidNsid,
    isValidRkey,
} from "@atproto/syntax";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AtUri` | `class` | Fields: `authority: string, collection: string, rkey: string, is_did: bool, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidAtUri` | `(): AtUri` | Invokes `invalidAtUri` and returns `AtUri`. |
| `isLowerAlpha` | `(b: u8): bool` | Checks predicate `isLowerAlpha` and returns `bool`. |
| `isAlpha` | `(b: u8): bool` | Checks predicate `isAlpha` and returns `bool`. |
| `isDigit` | `(b: u8): bool` | Checks predicate `isDigit` and returns `bool`. |
| `isAlphaNum` | `(b: u8): bool` | Checks predicate `isAlphaNum` and returns `bool`. |
| `isValidNsid` | `(nsid: string): bool` | Checks predicate `isValidNsid` and returns `bool`. |
| `isValidRkey` | `(rkey: string): bool` | Checks predicate `isValidRkey` and returns `bool`. |
| `isValidDid` | `(s: string): bool` | Checks predicate `isValidDid` and returns `bool`. |
| `isValidHandle` | `(s: string): bool` | Checks predicate `isValidHandle` and returns `bool`. |
| `parseAtUri` | `(uri: string): AtUri` | Parses input via `parseAtUri` and returns `AtUri`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `formatAtUri` | `(authority: string, collection: string, rkey: string, out: []u8): usize` | Serializes/formats data via `formatAtUri` returning `usize`. |

## Minimal Example

```cp
import { isValidNsid } from "@atproto/syntax";

pub fn main() {
    let result = isValidNsid("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
