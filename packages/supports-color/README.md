[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# supports-color

Zero-allocation terminal color capability detection (supports-color v9 parity) powered by has-flag in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:supports-color` ([https://github.com/chalk/supports-color](https://github.com/chalk/supports-color))

| Metadata | Value |
| --- | --- |
| Package | `supports-color` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add supports-color
```

```cp
import {
    ColorSupport,
    TerminalEnv,
    scContainsIgnoreCase,
    translateLevel,
    scDetectCliOverride,
    scParseForceColor,
    detectColorLevel,
    detectColorSupport,
} from "supports-color";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ColorSupport` | `class` | Fields: `level: u8, has_basic: bool, has_256: bool, has_16m: bool` |
| `TerminalEnv` | `class` | Fields: `is_tty: bool, force_color: string, no_color: string, term: string, colorterm: string, term_program: string, +4 more` · Methods: `empty, tty` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `scContainsIgnoreCase` | `(haystack: string, needle: string): bool` | Invokes `scContainsIgnoreCase` and returns `bool`. |
| `translateLevel` | `(level: u8): ColorSupport` | Invokes `translateLevel` and returns `ColorSupport`. |
| `scDetectCliOverride` | `(argv: &[]string): i8` | Invokes `scDetectCliOverride` and returns `i8`. |
| `scParseForceColor` | `(val: string): i8` | Invokes `scParseForceColor` and returns `i8`. |
| `detectColorLevel` | `(env: TerminalEnv, argv: &[]string): u8` | Invokes `detectColorLevel` and returns `u8`. |
| `detectColorSupport` | `(env: TerminalEnv, argv: &[]string): ColorSupport` | Invokes `detectColorSupport` and returns `ColorSupport`. |

## Minimal Example

```cp
import { scContainsIgnoreCase } from "supports-color";

pub fn main() {
    let result = scContainsIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
