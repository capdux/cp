[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# levn

Zero-heap Light Value Notation (levn) scalar, list, and object parser

| Metadata | Value |
| --- | --- |
| Package | `levn` |
| Version | `0.4.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add levn
```

```cp
import {
    LevnKind,
    LevnValue,
    LevnPair,
    lvnMakeInvalid,
    lvnTryParseNumber,
    parseLevnScalar,
    lvnFindTopLevelChar,
    lvnUnquoteKey,
} from "levn";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `LevnKind` | `Null, Bool, Int, Float, String, Invalid` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LevnValue` | `class` | Fields: `kind: LevnKind, bool_val: bool, int_val: i64, float_val: f64, str_val: string` |
| `LevnPair` | `class` | Fields: `key: string, value: LevnValue` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lvnMakeInvalid` | `(): LevnValue` | Invokes `lvnMakeInvalid` and returns `LevnValue`. |
| `lvnTryParseNumber` | `(s: string): LevnValue` | Invokes `lvnTryParseNumber` and returns `LevnValue`. |
| `parseLevnScalar` | `(input: string): LevnValue` | Parses input via `parseLevnScalar` and returns `LevnValue`. |
| `lvnFindTopLevelChar` | `(s: string, start: usize, target: u8): usize` | Invokes `lvnFindTopLevelChar` and returns `usize`. |
| `lvnUnquoteKey` | `(s: string): string` | Invokes `lvnUnquoteKey` and returns `string`. |
| `parseLevnList` | `(input: string, out: []LevnValue): usize` | Parses input via `parseLevnList` and returns `usize`. |
| `parseLevnObject` | `(input: string, out: []LevnPair): usize` | Parses input via `parseLevnObject` and returns `usize`. |

## Minimal Example

```cp
import { parseLevnScalar } from "levn";

pub fn main() {
    let result = parseLevnScalar("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
