[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# range-parser

Zero-heap RFC 7233 HTTP Range header parser with suffix ranges and interval combining (clean-room port of npm range-parser)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:range-parser` ([https://github.com/jshttp/range-parser](https://github.com/jshttp/range-parser))

| Metadata | Value |
| --- | --- |
| Package | `range-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add range-parser
```

```cp
import { ByteRange, RangeResult, parseRange, rangeByteLength } from "range-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ByteRange` | `class` | Fields: `start: u64, end: u64` |
| `RangeResult` | `class` | Fields: `status: i32, unit: string, ranges: [32]ByteRange, count: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseRange` | `(size: u64, header: string, combine: bool): RangeResult` | Parses input via `parseRange` and returns `RangeResult`. |
| `rangeByteLength` | `(r: ByteRange): u64` | Invokes `rangeByteLength` and returns `u64`. |

## Minimal Example

```cp
import { parseRange } from "range-parser";

pub fn main() {
    let result = parseRange(0, "example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
