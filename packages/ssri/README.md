[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ssri

Zero-allocation W3C Subresource Integrity (SRI) parser, validator, constant-time matcher, and formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ssri` ([https://github.com/npm/ssri](https://github.com/npm/ssri))

| Metadata | Value |
| --- | --- |
| Package | `ssri` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ssri
```

```cp
import {
    SriAlgo,
    SriHash,
    SriIntegrity,
    emptyHash,
    isValidAlgoName,
    parseAlgo,
    algoName,
    algoStrength,
} from "ssri";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SriAlgo` | `Sha256, Sha384, Sha512, Blake3, Unknown` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SriHash` | `class` | Fields: `valid: bool, algo: SriAlgo, algo_name: string, digest: string, options: string` |
| `SriIntegrity` | `class` | Fields: `hashes: [MAX_HASHES]SriHash, count: usize` · Methods: `get, isEmpty` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyHash` | `(): SriHash` | Invokes `emptyHash` and returns `SriHash`. |
| `isValidAlgoName` | `(name: string): bool` | Checks predicate `isValidAlgoName` and returns `bool`. |
| `parseAlgo` | `(name: string): SriAlgo` | Parses input via `parseAlgo` and returns `SriAlgo`. |
| `algoName` | `(algo: SriAlgo): string` | Invokes `algoName` and returns `string`. |
| `algoStrength` | `(algo: SriAlgo): u8` | Invokes `algoStrength` and returns `u8`. |
| `isBase64DataChar` | `(c: u8): bool` | Checks predicate `isBase64DataChar` and returns `bool`. |
| `isValidBase64` | `(s: string): bool` | Checks predicate `isValidBase64` and returns `bool`. |
| `isValidOptions` | `(opts: string): bool` | Checks predicate `isValidOptions` and returns `bool`. |
| `parseSingleToken` | `(tok: string): SriHash` | Parses input via `parseSingleToken` and returns `SriHash`. |
| `parseIntegrity` | `(raw: string): SriIntegrity` | Parses input via `parseIntegrity` and returns `SriIntegrity`. |
| `strongestHash` | `(sri: SriIntegrity): SriHash?` | Invokes `strongestHash` and returns `SriHash?`. |
| `pickAlgorithm` | `(sri: SriIntegrity, algo: SriAlgo): SriHash?` | Invokes `pickAlgorithm` and returns `SriHash?`. |
| `matchesIntegrity` | `(expected: SriIntegrity, actual: SriIntegrity): bool` | Invokes `matchesIntegrity` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `formatHash` | `(algo: SriAlgo, base64_digest: string, out: []u8): string` | Serializes/formats data via `formatHash` returning `string`. |
| `formatIntegrity` | `(sri: SriIntegrity, out: []u8): string` | Serializes/formats data via `formatIntegrity` returning `string`. |

## Minimal Example

```cp
import { isValidAlgoName } from "ssri";

pub fn main() {
    let result = isValidAlgoName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
