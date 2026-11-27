[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @opentelemetry/core

Zero-allocation W3C Baggage (OpenTelemetry) HTTP header parser, percent-codec, and serializer in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@opentelemetry/core` ([https://github.com/open-telemetry/opentelemetry-js](https://github.com/open-telemetry/opentelemetry-js))（原库名 `@opentelemetry/core` → CP 包名 `@opentelemetry/core`）

| Metadata | Value |
| --- | --- |
| Package | `@opentelemetry/core` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @opentelemetry/core
```

```cp
import {
    BaggageMember,
    isSpace,
    isTchar,
    isValidKey,
    trimSlice,
    strEq,
    parseSingleMember,
    parseBaggage,
} from "@opentelemetry/core";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BaggageMember` | `class` | Fields: `key: string, value: string, properties: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSpace` | `(b: u8): bool` | Checks predicate `isSpace` and returns `bool`. |
| `isTchar` | `(b: u8): bool` | Checks predicate `isTchar` and returns `bool`. |
| `isValidKey` | `(key: string): bool` | Checks predicate `isValidKey` and returns `bool`. |
| `trimSlice` | `(s: string): string` | Invokes `trimSlice` and returns `string`. |
| `strEq` | `(a: string, b: string): bool` | Invokes `strEq` and returns `bool`. |
| `parseSingleMember` | `(seg: string, out: &mut BaggageMember): bool` | Parses input via `parseSingleMember` and returns `bool`. |
| `parseBaggage` | `(header: string, out: []BaggageMember): usize` | Parses input via `parseBaggage` and returns `usize`. |
| `countMembers` | `(header: string): usize` | Invokes `countMembers` and returns `usize`. |
| `hexVal` | `(b: u8): i32` | Invokes `hexVal` and returns `i32`. |
| `percentDecode` | `(src: string, out: []u8, out_len: &mut usize): bool` | Invokes `percentDecode` and returns `bool`. |
| `getDecodedValue` | `(header: string, target_key: string, out: []u8, out_len: &mut usize): bool` | Queries/retrieves data via `getDecodedValue` returning `bool`. |
| `hexUpper` | `(nibble: u8): u8` | Invokes `hexUpper` and returns `u8`. |
| `needsPercentEncode` | `(b: u8): bool` | Invokes `needsPercentEncode` and returns `bool`. |
| `formatBaggage` | `(members: &[]BaggageMember, out: []u8): usize` | Serializes/formats data via `formatBaggage` returning `usize`. |

## Minimal Example

```cp
import { isValidKey } from "@opentelemetry/core";

pub fn main() {
    let result = isValidKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
