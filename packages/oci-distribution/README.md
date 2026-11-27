[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# oci-distribution

Zero-allocation OCI / Docker image reference parser, validator, and normalizer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oci-distribution` ([https://github.com/krustlet/oci-distribution](https://github.com/krustlet/oci-distribution)) / `npm:docker-image-parse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `oci-distribution` → CP 包名 `oci-distribution`）

| Metadata | Value |
| --- | --- |
| Package | `oci-distribution` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add oci-distribution
```

```cp
import {
    ImageRef,
    drIsLower,
    drIsUpper,
    drIsDigit,
    drIsLowerHex,
    drEqBytes,
    drContainsByte,
    isValidTag,
} from "oci-distribution";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ImageRef` | `class` | Fields: `registry: string, repository: string, tag: string, digest: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `drIsLower` | `(c: u8): bool` | Invokes `drIsLower` and returns `bool`. |
| `drIsUpper` | `(c: u8): bool` | Invokes `drIsUpper` and returns `bool`. |
| `drIsDigit` | `(c: u8): bool` | Invokes `drIsDigit` and returns `bool`. |
| `drIsLowerHex` | `(c: u8): bool` | Invokes `drIsLowerHex` and returns `bool`. |
| `drEqBytes` | `(a: &[]u8, b: &[]u8): bool` | Invokes `drEqBytes` and returns `bool`. |
| `drContainsByte` | `(s: &[]u8, target: u8): bool` | Invokes `drContainsByte` and returns `bool`. |
| `isValidTag` | `(tag: string): bool` | Checks predicate `isValidTag` and returns `bool`. |
| `isValidDigest` | `(digest: string): bool` | Checks predicate `isValidDigest` and returns `bool`. |
| `drIsValidRegistry` | `(reg: &[]u8): bool` | Invokes `drIsValidRegistry` and returns `bool`. |
| `drIsValidRepository` | `(repo: &[]u8): bool` | Invokes `drIsValidRepository` and returns `bool`. |
| `parseImageRef` | `(input: string): ImageRef` | Parses input via `parseImageRef` and returns `ImageRef`. |
| `drWrite` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `drWrite` and returns `bool`. |
| `formatNormalized` | `(ref_in: ImageRef, out: []u8): usize` | Serializes/formats data via `formatNormalized` returning `usize`. |

## Minimal Example

```cp
import { isValidTag } from "oci-distribution";

pub fn main() {
    let result = isValidTag("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
