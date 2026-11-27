[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shell-quote

Zero-heap POSIX shell argument quoting and command-line tokenization in pure CP (modeled after npm shell-quote)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-quote` ([https://github.com/ljharb/shell-quote](https://github.com/ljharb/shell-quote))

| Metadata | Value |
| --- | --- |
| Package | `shell-quote` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shell-quote
```

```cp
import {
    ParsedArgSpan,
    SplitResult,
    isSafeBareChar,
    needsQuoting,
    quoteArg,
    quoteArgs,
    splitCommand,
} from "shell-quote";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParsedArgSpan` | `class` | Fields: `start: usize, len: usize` |
| `SplitResult` | `class` | Fields: `arg_count: usize, bytes_used: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSafeBareChar` | `(c: u8): bool` | Checks predicate `isSafeBareChar` and returns `bool`. |
| `needsQuoting` | `(arg: &[]u8): bool` | Invokes `needsQuoting` and returns `bool`. |
| `quoteArg` | `(arg: &[]u8, dst: []u8): i64` | Invokes `quoteArg` and returns `i64`. |
| `quoteArgs` | `(args: &[]&[]u8, dst: []u8): i64` | Invokes `quoteArgs` and returns `i64`. |
| `splitCommand` | `(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult` | Invokes `splitCommand` and returns `SplitResult`. |

## Minimal Example

```cp
import { isSafeBareChar } from "shell-quote";

pub fn main() {
    let result = isSafeBareChar(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
