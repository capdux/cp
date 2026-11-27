[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# atoi

Zero-heap overflow-checked integer parsing directly from byte slices (base 10, 16, 2..36) matching Rust atoi (262M+ downloads) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:atoi` ([https://github.com/pacman82/atoi-rs](https://github.com/pacman82/atoi-rs))

| Metadata | Value |
| --- | --- |
| Package | `atoi` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add atoi
```

```cp
import {
    ParseU64Result,
    ParseI64Result,
    digitVal,
    fromRadixU64,
    fromRadix10U64,
    fromRadix16U64,
    fromRadixI64,
    fromRadix10I64,
} from "atoi";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParseU64Result` | `class` | Fields: `value: u64, used: usize, ok: bool` |
| `ParseI64Result` | `class` | Fields: `value: i64, used: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `digitVal` | `(ch: u8, radix: u32): i32` | Invokes `digitVal` and returns `i32`. |
| `fromRadixU64` | `(bytes: string, radix: u32): ParseU64Result` | Invokes `fromRadixU64` and returns `ParseU64Result`. |
| `fromRadix10U64` | `(bytes: string): ParseU64Result` | Invokes `fromRadix10U64` and returns `ParseU64Result`. |
| `fromRadix16U64` | `(bytes: string): ParseU64Result` | Invokes `fromRadix16U64` and returns `ParseU64Result`. |
| `fromRadixI64` | `(bytes: string, radix: u32): ParseI64Result` | Invokes `fromRadixI64` and returns `ParseI64Result`. |
| `fromRadix10I64` | `(bytes: string): ParseI64Result` | Invokes `fromRadix10I64` and returns `ParseI64Result`. |
| `atoiU64` | `(s: string): ParseU64Result` | Invokes `atoiU64` and returns `ParseU64Result`. |
| `atoiI64` | `(s: string): ParseI64Result` | Invokes `atoiI64` and returns `ParseI64Result`. |

## Minimal Example

```cp
import { fromRadixU64 } from "atoi";

pub fn main() {
    let result = fromRadixU64("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
