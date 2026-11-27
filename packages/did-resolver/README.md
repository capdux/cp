[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# did-resolver

Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:did-resolver` ([https://github.com/decentralized-identity/did-resolver](https://github.com/decentralized-identity/did-resolver)) / `crates.io:ssi-dids` ([https://github.com/spruceid/ssi](https://github.com/spruceid/ssi))（原库名 `did-resolver` → CP 包名 `did-resolver`）

| Metadata | Value |
| --- | --- |
| Package | `did-resolver` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add did-resolver
```

```cp
import {
    DidUrl,
    isValidMethod,
    isValidMethodId,
    parseDid,
    appendSlice,
    formatDid,
} from "did-resolver";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DidUrl` | `class` | Fields: `did: string, method: string, id: string, path: string, query: string, fragment: string, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValidMethod` | `(method: string): bool` | Checks predicate `isValidMethod` and returns `bool`. |
| `isValidMethodId` | `(id: string): bool` | Checks predicate `isValidMethodId` and returns `bool`. |
| `parseDid` | `(input: string): DidUrl` | Parses input via `parseDid` and returns `DidUrl`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `formatDid` | `(method: string, id: string, fragment: string, out: []u8): usize` | Serializes/formats data via `formatDid` returning `usize`. |

## Minimal Example

```cp
import { isValidMethod } from "did-resolver";

pub fn main() {
    let result = isValidMethod("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
