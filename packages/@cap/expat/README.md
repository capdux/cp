[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/expat

Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) native C binding for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/expat` |
| Version | `2.6.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/expat
```

```cp
import {
    ExpatError,
    cp_expat_version,
    cp_expat_count_elements,
    versionString,
    countElements,
} from "@cap/expat";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `ExpatError` | `InvalidInput, ParserInitFailed, ParseFailed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_expat_version` | `(): [*:0]&u8!` | Invokes `cp_expat_version` and returns `[*:0]&u8!`. |
| `cp_expat_count_elements` | `(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32` | Invokes `cp_expat_count_elements` and returns `i32`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |
| `countElements` | `(xml: &[]u8): !usize` | Invokes `countElements` and returns `!usize`. |

## Minimal Example

```cp
import { versionString } from "@cap/expat";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
