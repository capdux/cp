[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# vcard

Zero-allocation RFC 6350 vCard 4.0 (.vcf) contact generator and parser for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vcard` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `vcard` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add vcard
```

```cp
import {
    VCardData,
    ParsedVCard,
    isSpaceOrTab,
    writeRaw,
    writeEscaped,
    writeProp,
    formatVCard,
    parseVCard,
} from "vcard";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VCardData` | `class` | Fields: `full_name: &[]u8, org: &[]u8, title: &[]u8, email: &[]u8, tel: &[]u8, url: &[]u8` |
| `ParsedVCard` | `class` | Fields: `fn_start: usize, fn_len: usize, org_start: usize, org_len: usize, title_start: usize, title_len: usize, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSpaceOrTab` | `(c: u8): bool` | Checks predicate `isSpaceOrTab` and returns `bool`. |
| `writeRaw` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `writeRaw` and returns `bool`. |
| `writeEscaped` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `writeEscaped` and returns `bool`. |
| `writeProp` | `(dst: []u8, pos: &mut usize, propPrefix: &[]u8, val: &[]u8): bool` | Invokes `writeProp` and returns `bool`. |
| `formatVCard` | `(card: VCardData, dst: []u8): i64` | Serializes/formats data via `formatVCard` returning `i64`. |
| `parseVCard` | `(vcf: &[]u8): ParsedVCard` | Parses input via `parseVCard` and returns `ParsedVCard`. |

## Minimal Example

```cp
import { isSpaceOrTab } from "vcard";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
