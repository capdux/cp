[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# packageurl-js

Zero-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical serializer (SBOM standard) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:packageurl-js` ([https://github.com/package-url/packageurl-js](https://github.com/package-url/packageurl-js)) / `crates.io:packageurl` ([https://github.com/althonos/packageurl.rs](https://github.com/althonos/packageurl.rs))（原库名 `packageurl-js` → CP 包名 `packageurl-js`）

| Metadata | Value |
| --- | --- |
| Package | `packageurl-js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add packageurl-js
```

```cp
import {
    Purl,
    invalidPurl,
    purlIsSpace,
    isValidType,
    trimSlashes,
    parsePurl,
    getQualifier,
    pushSlice,
} from "packageurl-js";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Purl` | `class` | Fields: `pkg_type: string, namespace: string, name: string, version: string, qualifiers: string, subpath: string, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidPurl` | `(): Purl` | Invokes `invalidPurl` and returns `Purl`. |
| `purlIsSpace` | `(b: u8): bool` | Invokes `purlIsSpace` and returns `bool`. |
| `isValidType` | `(pkg_type: string): bool` | Checks predicate `isValidType` and returns `bool`. |
| `trimSlashes` | `(s: string): string` | Invokes `trimSlashes` and returns `string`. |
| `parsePurl` | `(input: string): Purl` | Parses input via `parsePurl` and returns `Purl`. |
| `getQualifier` | `(purl: Purl, key: string): string` | Queries/retrieves data via `getQualifier` returning `string`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushLowerSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushLowerSlice`. |
| `formatPurl` | `(p: Purl, out: []u8): usize` | Serializes/formats data via `formatPurl` returning `usize`. |

## Minimal Example

```cp
import { isValidType } from "packageurl-js";

pub fn main() {
    let result = isValidType("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
