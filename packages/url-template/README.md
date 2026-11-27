[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# url-template

Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and validator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-template` ([https://github.com/bramstein/url-template](https://github.com/bramstein/url-template))（原库名 `url-template` → CP 包名 `url-template`）

| Metadata | Value |
| --- | --- |
| Package | `url-template` |
| Version | `0.1.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add url-template
```

```cp
import {
    TemplateVar,
    utIsAlpha,
    utIsDigit,
    utIsHex,
    utHexUpper,
    utIsUnreserved,
    utIsReserved,
    utIsOperator,
} from "url-template";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TemplateVar` | `class` | Fields: `name: string, value: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `utIsAlpha` | `(c: u8): bool` | Invokes `utIsAlpha` and returns `bool`. |
| `utIsDigit` | `(c: u8): bool` | Invokes `utIsDigit` and returns `bool`. |
| `utIsHex` | `(c: u8): bool` | Invokes `utIsHex` and returns `bool`. |
| `utHexUpper` | `(n: u8): u8` | Invokes `utHexUpper` and returns `u8`. |
| `utIsUnreserved` | `(c: u8): bool` | Invokes `utIsUnreserved` and returns `bool`. |
| `utIsReserved` | `(c: u8): bool` | Invokes `utIsReserved` and returns `bool`. |
| `utIsOperator` | `(c: u8): bool` | Invokes `utIsOperator` and returns `bool`. |
| `utIsReservedOp` | `(c: u8): bool` | Invokes `utIsReservedOp` and returns `bool`. |
| `utEqBytes` | `(a: &[]u8, b: &[]u8): bool` | Invokes `utEqBytes` and returns `bool`. |
| `utIsValidVarSpec` | `(spec: &[]u8): bool` | Invokes `utIsValidVarSpec` and returns `bool`. |
| `isValid` | `(tmpl: string): bool` | Checks predicate `isValid` and returns `bool`. |
| `countVars` | `(tmpl: string): usize` | Invokes `countVars` and returns `usize`. |
| `extractVarName` | `(tmpl: string, index: usize, out: []u8): usize` | Invokes `extractVarName` and returns `usize`. |
| `pctEncode` | `(input: string, allow_reserved: bool, out: []u8): usize` | Invokes `pctEncode` and returns `usize`. |
| `utTruncateUtf8` | `(val: &[]u8, maxChars: usize): &[]u8` | Invokes `utTruncateUtf8` and returns `&[]u8`. |
| `utWriteSlice` | `(out: []u8, pos: &mut usize, src: &[]u8)` | Executes `utWriteSlice` in-place. |
| `utWriteByte` | `(out: []u8, pos: &mut usize, b: u8)` | Executes `utWriteByte` in-place. |
| `expand` | `(tmpl: string, vars: &[]TemplateVar, out: []u8): usize` | Invokes `expand` and returns `usize`. |

## Minimal Example

```cp
import { isValid } from "url-template";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
