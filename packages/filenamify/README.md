[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# filenamify

Zero-allocation cross-platform safe filename sanitizer (npm filenamify v6 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:filenamify` ([https://github.com/sindresorhus/filenamify](https://github.com/sindresorhus/filenamify))

| Metadata | Value |
| --- | --- |
| Package | `filenamify` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add filenamify
```

```cp
import {
    fnmIsInvalidChar,
    fnmHasInvalidChar,
    fnmToLowerAscii,
    fnmEq3IgnoreCase,
    fnmMatchAt,
    fnmEndsWith,
    fnmAppendStr,
    fnmUtf8SafeLen,
} from "filenamify";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fnmIsInvalidChar` | `(ch: u8): bool` | Invokes `fnmIsInvalidChar` and returns `bool`. |
| `fnmHasInvalidChar` | `(s: string): bool` | Invokes `fnmHasInvalidChar` and returns `bool`. |
| `fnmToLowerAscii` | `(ch: u8): u8` | Invokes `fnmToLowerAscii` and returns `u8`. |
| `fnmEq3IgnoreCase` | `(s: string, a: u8, b: u8, c: u8): bool` | Invokes `fnmEq3IgnoreCase` and returns `bool`. |
| `fnmMatchAt` | `(buf: string, pos: usize, pat: string): bool` | Invokes `fnmMatchAt` and returns `bool`. |
| `fnmEndsWith` | `(buf: string, len: usize, pat: string): bool` | Invokes `fnmEndsWith` and returns `bool`. |
| `fnmAppendStr` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `fnmAppendStr` and returns `usize`. |
| `fnmUtf8SafeLen` | `(s: string, max_len: usize): usize` | Invokes `fnmUtf8SafeLen` and returns `usize`. |
| `isReservedFilename` | `(name: string): bool` | Checks predicate `isReservedFilename` and returns `bool`. |
| `filenamifyWithOptions` | `(input: string, replacement: string, max_len: usize, out: []u8): usize` | Invokes `filenamifyWithOptions` and returns `usize`. |
| `filenamify` | `(input: string, out: []u8): usize` | Invokes `filenamify` and returns `usize`. |

## Minimal Example

```cp
import { isReservedFilename } from "filenamify";

pub fn main() {
    let result = isReservedFilename("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
