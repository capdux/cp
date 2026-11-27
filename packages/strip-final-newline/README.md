[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strip-final-newline

Zero-copy final LF/CRLF newline stripper and normalizer matching npm strip-final-newline (250M+/wk) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-final-newline` ([https://github.com/sindresorhus/strip-final-newline](https://github.com/sindresorhus/strip-final-newline))

| Metadata | Value |
| --- | --- |
| Package | `strip-final-newline` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strip-final-newline
```

```cp
import {
    stripFinalNewline,
    stripAllTrailingNewlines,
    hasFinalNewline,
    hasFinalCrlf,
    ensureFinalNewline,
} from "strip-final-newline";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `stripFinalNewline` | `(input: string): string` | Invokes `stripFinalNewline` and returns `string`. |
| `stripAllTrailingNewlines` | `(input: string): string` | Invokes `stripAllTrailingNewlines` and returns `string`. |
| `hasFinalNewline` | `(input: string): bool` | Checks predicate `hasFinalNewline` and returns `bool`. |
| `hasFinalCrlf` | `(input: string): bool` | Checks predicate `hasFinalCrlf` and returns `bool`. |
| `ensureFinalNewline` | `(input: string, use_crlf: bool, out: []u8): usize` | Invokes `ensureFinalNewline` and returns `usize`. |

## Minimal Example

```cp
import { stripFinalNewline } from "strip-final-newline";

pub fn main() {
    let result = stripFinalNewline("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
