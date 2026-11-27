[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ansi-regex

Zero-heap ANSI escape sequence matcher & locator reusing anstyle-parse, matching npm ansi-regex (400M+/wk) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-regex` ([https://github.com/chalk/ansi-regex](https://github.com/chalk/ansi-regex))

| Metadata | Value |
| --- | --- |
| Package | `ansi-regex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ansi-regex
```

```cp
import {
    AnsiSpan,
    testAnsi,
    findFirstAnsi,
    findAllAnsi,
    countAnsiSequences,
    stripAllAnsi,
} from "ansi-regex";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AnsiSpan` | `class` | Fields: `start: usize, len: usize, found: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `testAnsi` | `(input: string): bool` | Invokes `testAnsi` and returns `bool`. |
| `findFirstAnsi` | `(input: string): AnsiSpan` | Queries/retrieves data via `findFirstAnsi` returning `AnsiSpan`. |
| `findAllAnsi` | `(input: string, out: []AnsiSpan): usize` | Queries/retrieves data via `findAllAnsi` returning `usize`. |
| `countAnsiSequences` | `(input: string): usize` | Invokes `countAnsiSequences` and returns `usize`. |
| `stripAllAnsi` | `(input: string, out: []u8): usize` | Invokes `stripAllAnsi` and returns `usize`. |

## Minimal Example

```cp
import { testAnsi } from "ansi-regex";

pub fn main() {
    let result = testAnsi("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
