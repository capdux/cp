[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# phonenumber

Zero-heap RFC 3966 tel: URI parser, E.164 international phone number normalizer/validator, and URI formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:phonenumber` ([https://github.com/whisperfish/rust-phonenumber](https://github.com/whisperfish/rust-phonenumber))（原库名 `phonenumber` → CP 包名 `phonenumber`）

| Metadata | Value |
| --- | --- |
| Package | `phonenumber` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add phonenumber
```

```cp
import {
    TelUri,
    isVisualSep,
    isWs,
    isValidE164,
    normalizeE164,
    parseTelUri,
    appendSlice,
    formatTelUri,
} from "phonenumber";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TelUri` | `class` | Fields: `number: string, is_global: bool, ext: string, phone_context: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isVisualSep` | `(c: u8): bool` | Checks predicate `isVisualSep` and returns `bool`. |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `isValidE164` | `(number: string): bool` | Checks predicate `isValidE164` and returns `bool`. |
| `normalizeE164` | `(input: string, out: []u8): usize` | Invokes `normalizeE164` and returns `usize`. |
| `parseTelUri` | `(uri: string): TelUri` | Parses input via `parseTelUri` and returns `TelUri`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `formatTelUri` | `(e164_number: string, ext: string, out: []u8): usize` | Serializes/formats data via `formatTelUri` returning `usize`. |

## Minimal Example

```cp
import { isValidE164 } from "phonenumber";

pub fn main() {
    let result = isValidE164("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
