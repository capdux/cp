[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-slash

Zero-heap POSIX '/' and Windows '\' path separator conversion (UNC, drive letters, lossy UTF-8) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-slash` ([https://github.com/rhysd/path-slash](https://github.com/rhysd/path-slash))

| Metadata | Value |
| --- | --- |
| Package | `path-slash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add path-slash
```

```cp
import {
    pshIsAsciiAlpha,
    pshIsSep,
    pshWriteByte,
    pshValidUtf8Len,
    pshIsAbs,
    toSlash,
    toSlashLossy,
    fromSlash,
} from "path-slash";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pshIsAsciiAlpha` | `(b: u8): bool` | Invokes `pshIsAsciiAlpha` and returns `bool`. |
| `pshIsSep` | `(b: u8): bool` | Invokes `pshIsSep` and returns `bool`. |
| `pshWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `pshWriteByte` and returns `usize`. |
| `pshValidUtf8Len` | `(s: string, offset: usize): usize` | Invokes `pshValidUtf8Len` and returns `usize`. |
| `pshIsAbs` | `(path: string): bool` | Invokes `pshIsAbs` and returns `bool`. |
| `toSlash` | `(path: string, out: []u8): usize` | Invokes `toSlash` and returns `usize`. |
| `toSlashLossy` | `(path: string, out: []u8): usize` | Invokes `toSlashLossy` and returns `usize`. |
| `fromSlash` | `(path: string, out: []u8): usize` | Invokes `fromSlash` and returns `usize`. |
| `hasBackslash` | `(path: string): bool` | Checks predicate `hasBackslash` and returns `bool`. |
| `hasForwardSlash` | `(path: string): bool` | Checks predicate `hasForwardSlash` and returns `bool`. |
| `trimTrailingSlashes` | `(path: string): string` | Invokes `trimTrailingSlashes` and returns `string`. |
| `joinSlash` | `(a: string, b: string, out: []u8): usize` | Invokes `joinSlash` and returns `usize`. |

## Minimal Example

```cp
import { pshValidUtf8Len } from "path-slash";

pub fn main() {
    let result = pshValidUtf8Len("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
