[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ldapjs

Zero-heap RFC 4514 LDAP Distinguished Name parser, value escaper/unescaper, validator, and case-insensitive attribute lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ldapjs` ([https://github.com/ldapjs/node-ldapjs](https://github.com/ldapjs/node-ldapjs)) / `crates.io:ldap3` ([https://github.com/inejge/ldap3](https://github.com/inejge/ldap3))（原库名 `ldapjs` → CP 包名 `ldapjs`）

| Metadata | Value |
| --- | --- |
| Package | `ldapjs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ldapjs
```

```cp
import {
    RdnAttr,
    isWs,
    isValidAttrType,
    isEscapedAt,
    hasValidEscapes,
    parseDn,
    escapeValue,
    unescapeValue,
} from "ldapjs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RdnAttr` | `class` | Fields: `attr: string, raw_value: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `isValidAttrType` | `(attr: string): bool` | Checks predicate `isValidAttrType` and returns `bool`. |
| `isEscapedAt` | `(s: string, start: usize, idx: usize): bool` | Checks predicate `isEscapedAt` and returns `bool`. |
| `hasValidEscapes` | `(raw: string): bool` | Checks predicate `hasValidEscapes` and returns `bool`. |
| `parseDn` | `(dn: string, out: []RdnAttr): usize` | Parses input via `parseDn` and returns `usize`. |
| `escapeValue` | `(value: string, out: []u8): usize` | Invokes `escapeValue` and returns `usize`. |
| `unescapeValue` | `(raw: string, out: []u8): usize` | Invokes `unescapeValue` and returns `usize`. |
| `findAttr` | `(dn: string, target_attr: string, out: []u8, out_len: &mut usize): bool` | Queries/retrieves data via `findAttr` returning `bool`. |
| `isValidDn` | `(dn: string): bool` | Checks predicate `isValidDn` and returns `bool`. |

## Minimal Example

```cp
import { isValidAttrType } from "ldapjs";

pub fn main() {
    let result = isValidAttrType("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
