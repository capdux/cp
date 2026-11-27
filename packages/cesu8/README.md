[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cesu8

Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) to/from standard UTF-8 transcoder

| Metadata | Value |
| --- | --- |
| Package | `cesu8` |
| Version | `1.1.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cesu8
```

```cp
import { toCesu8, fromCesu8, isValidCesu8AsciiOrBmp } from "cesu8";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `toCesu8` | `(utf8: &[]u8, modified_nul: bool, out: []u8): i64` | Invokes `toCesu8` and returns `i64`. |
| `fromCesu8` | `(cesu: &[]u8, modified_nul: bool, out: []u8): i64` | Invokes `fromCesu8` and returns `i64`. |
| `isValidCesu8AsciiOrBmp` | `(utf8: &[]u8, modified_nul: bool): bool` | Checks predicate `isValidCesu8AsciiOrBmp` and returns `bool`. |

## Minimal Example

```cp
import { isValidCesu8AsciiOrBmp } from "cesu8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidCesu8AsciiOrBmp(out_buf[0..256], false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
