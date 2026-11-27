[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# env_filter

Zero-allocation RUST_LOG-style hierarchical log filter parser and longest-prefix module matcher (Rust env_filter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:env_filter` ([https://github.com/rust-cli/env_logger](https://github.com/rust-cli/env_logger))

| Metadata | Value |
| --- | --- |
| Package | `env_filter` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add env_filter
```

```cp
import {
    LogLevel,
    FilterDirective,
    EnvFilter,
    efltLevelRank,
    parseLogLevel,
    efltMatchesTarget,
} from "env_filter";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `LogLevel` | `Off, Error, Warn, Info, Debug, Trace` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FilterDirective` | `class` | Fields: `target: string, level: LogLevel` |
| `EnvFilter` | `class` | Fields: `directives: [32]FilterDirective, count: usize` · Methods: `empty, sortDesc, addDirective, parse, directiveCount, getDirective, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `efltLevelRank` | `(lvl: LogLevel): u8` | Invokes `efltLevelRank` and returns `u8`. |
| `parseLogLevel` | `(s: string): LogLevel?` | Parses input via `parseLogLevel` and returns `LogLevel?`. |
| `efltMatchesTarget` | `(target: string, directive_target: string): bool` | Invokes `efltMatchesTarget` and returns `bool`. |

## Minimal Example

```cp
import { parseLogLevel } from "env_filter";

pub fn main() {
    let result = parseLogLevel("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
