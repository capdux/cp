[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# binary-extensions

Zero-heap O(log n) lookup of 127 common binary file extensions and categories in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:binary-extensions` ([https://github.com/sindresorhus/binary-extensions](https://github.com/sindresorhus/binary-extensions))

| Metadata | Value |
| --- | --- |
| Package | `binary-extensions` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add binary-extensions
```

```cp
import { cmpLowerToAscii, extensionCategory, isBinaryExtension, isBinaryPath } from "binary-extensions";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmpLowerToAscii` | `(a: string, b: string): i32` | Invokes `cmpLowerToAscii` and returns `i32`. |
| `extensionCategory` | `(ext: string): u8` | Invokes `extensionCategory` and returns `u8`. |
| `isBinaryExtension` | `(ext: string): bool` | Checks predicate `isBinaryExtension` and returns `bool`. |
| `isBinaryPath` | `(path: string): bool` | Checks predicate `isBinaryPath` and returns `bool`. |

## Minimal Example

```cp
import { isBinaryExtension } from "binary-extensions";

pub fn main() {
    let result = isBinaryExtension("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
