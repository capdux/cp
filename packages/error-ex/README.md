[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# error-ex

Zero-heap structured error and context formatter with template interpolation

| Metadata | Value |
| --- | --- |
| Package | `error-ex` |
| Version | `1.3.2` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add error-ex
```

```cp
import {
    ErrorField,
    StructuredError,
    erexWriteByte,
    erexWriteStr,
    erexWriteI32,
    formatStructuredError,
    erexMatchesPlaceholder,
    formatErrorTemplate,
} from "error-ex";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ErrorField` | `class` | Fields: `key: string, value: string, line_mode: bool` |
| `StructuredError` | `class` | Fields: `name: string, code: i32, message: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `erexWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `erexWriteByte` and returns `usize`. |
| `erexWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `erexWriteStr` and returns `usize`. |
| `erexWriteI32` | `(out: []u8, pos: usize, val: i32): usize` | Invokes `erexWriteI32` and returns `usize`. |
| `formatStructuredError` | `(e: StructuredError, fields: &[]ErrorField, out: []u8): usize` | Serializes/formats data via `formatStructuredError` returning `usize`. |
| `erexMatchesPlaceholder` | `(tmpl: string, start: usize, key: string): bool` | Invokes `erexMatchesPlaceholder` and returns `bool`. |
| `formatErrorTemplate` | `(tmpl: string, key: string, val: string, out: []u8): usize` | Serializes/formats data via `formatErrorTemplate` returning `usize`. |

## Minimal Example

```cp
import { formatErrorTemplate } from "error-ex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatErrorTemplate("example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
