[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# anstyle-parse

Zero-heap Paul Williams VT100/ANSI escape sequence state-machine parser (CSI/OSC/ESC/SGR) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle-parse` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:anstream` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle))

| Metadata | Value |
| --- | --- |
| Package | `anstyle-parse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add anstyle-parse
```

```cp
import {
    AnsiToken,
    SgrParams,
    nextToken,
    tokenizeAnsi,
    parseCsiParams,
    stripAnsiToBuf,
    hasAnsi,
    visibleByteLen,
} from "anstyle-parse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AnsiToken` | `class` | Fields: `kind: u8, start: usize, len: usize, final_byte: u8` |
| `SgrParams` | `class` | Fields: `params: [16]u16, count: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nextToken` | `(input: string, pos: usize): AnsiToken` | Invokes `nextToken` and returns `AnsiToken`. |
| `tokenizeAnsi` | `(input: string, out: []AnsiToken): usize` | Invokes `tokenizeAnsi` and returns `usize`. |
| `parseCsiParams` | `(csi_seq: string): SgrParams` | Parses input via `parseCsiParams` and returns `SgrParams`. |
| `stripAnsiToBuf` | `(input: string, out: []u8): usize` | Invokes `stripAnsiToBuf` and returns `usize`. |
| `hasAnsi` | `(input: string): bool` | Checks predicate `hasAnsi` and returns `bool`. |
| `visibleByteLen` | `(input: string): usize` | Invokes `visibleByteLen` and returns `usize`. |

## Minimal Example

```cp
import { parseCsiParams } from "anstyle-parse";

pub fn main() {
    let result = parseCsiParams("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
