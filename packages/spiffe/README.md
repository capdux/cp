[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# spiffe

Zero-allocation CNCF SPIFFE ID (spiffe://<trust-domain>/<path>) parser, strict security validator, and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:spiffe` ([https://github.com/maxlambrecht/rust-spiffe](https://github.com/maxlambrecht/rust-spiffe))（原库名 `spiffe` → CP 包名 `spiffe`）

| Metadata | Value |
| --- | --- |
| Package | `spiffe` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add spiffe
```

```cp
import {
    SpiffeId,
    sidEqBytes,
    sidIsTrustDomainChar,
    sidIsPathSegmentChar,
    isValidTrustDomain,
    isValidPath,
    parseSpiffeId,
    belongsToTrustDomain,
} from "spiffe";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SpiffeId` | `class` | Fields: `trust_domain: string, path: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sidEqBytes` | `(a: &[]u8, b: &[]u8): bool` | Invokes `sidEqBytes` and returns `bool`. |
| `sidIsTrustDomainChar` | `(c: u8): bool` | Invokes `sidIsTrustDomainChar` and returns `bool`. |
| `sidIsPathSegmentChar` | `(c: u8): bool` | Invokes `sidIsPathSegmentChar` and returns `bool`. |
| `isValidTrustDomain` | `(td: string): bool` | Checks predicate `isValidTrustDomain` and returns `bool`. |
| `isValidPath` | `(path: string): bool` | Checks predicate `isValidPath` and returns `bool`. |
| `parseSpiffeId` | `(uri: string): SpiffeId` | Parses input via `parseSpiffeId` and returns `SpiffeId`. |
| `belongsToTrustDomain` | `(id: SpiffeId, trust_domain: string): bool` | Invokes `belongsToTrustDomain` and returns `bool`. |
| `formatSpiffeId` | `(trust_domain: string, path: string, out: []u8): usize` | Serializes/formats data via `formatSpiffeId` returning `usize`. |

## Minimal Example

```cp
import { isValidTrustDomain } from "spiffe";

pub fn main() {
    let result = isValidTrustDomain("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
