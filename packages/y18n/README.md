[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# y18n

Zero-allocation internationalization (i18n) catalog and printf-style formatter (y18n parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:y18n` ([https://github.com/yargs/y18n](https://github.com/yargs/y18n))

| Metadata | Value |
| --- | --- |
| Package | `y18n` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add y18n
```

```cp
import { Y18nEntry, Y18nCatalog, y18WriteByte, formatPrintf } from "y18n";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Y18nEntry` | `class` | Fields: `key: string, singular: string, plural: string, has_plural: bool` |
| `Y18nCatalog` | `class` | Fields: `locale: string, count: usize, entries: [64]Y18nEntry` · Methods: `new, setLocale, getLocale, clear, findIndex, hasEntry, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `y18WriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `y18WriteByte` and returns `usize`. |
| `formatPrintf` | `(tmpl: string, count: i64, has_count: bool, args: &[]string, out: []u8): usize` | Serializes/formats data via `formatPrintf` returning `usize`. |

## Minimal Example

```cp
import { formatPrintf } from "y18n";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrintf("example", 0, false, /* args: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
