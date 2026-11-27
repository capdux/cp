[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cargo-platform

Zero-allocation LLVM/Rust target triple parser and Cargo cfg(...) expression evaluator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cargo-platform` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo))

| Metadata | Value |
| --- | --- |
| Package | `cargo-platform` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cargo-platform
```

```cp
import {
    TargetTriple,
    TargetEnv,
    CfgEvalResult,
    CfgParser,
    invalidTriple,
    parseTargetTriple,
    isIdentStart,
    isIdentCont,
} from "cargo-platform";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TargetTriple` | `class` | Fields: `arch_start: usize, arch_len: usize, vendor_start: usize, vendor_len: usize, os_start: usize, os_len: usize, +4 more` |
| `TargetEnv` | `class` | Fields: `arch: string, os: string, family: string, env: string, pointer_width: u32, is_big_endian: bool` |
| `CfgEvalResult` | `class` | Fields: `matched: bool, ok: bool` |
| `CfgParser` | `class` | Fields: `pos: usize, ok: bool, depth: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidTriple` | `(): TargetTriple` | Invokes `invalidTriple` and returns `TargetTriple`. |
| `parseTargetTriple` | `(triple: string): TargetTriple` | Parses input via `parseTargetTriple` and returns `TargetTriple`. |
| `isIdentStart` | `(c: u8): bool` | Checks predicate `isIdentStart` and returns `bool`. |
| `isIdentCont` | `(c: u8): bool` | Checks predicate `isIdentCont` and returns `bool`. |
| `skipWs` | `(p: &mut CfgParser, src: string)` | Executes `skipWs` in-place. |
| `u32MatchesDecimal` | `(n: u32, s: string): bool` | Invokes `u32MatchesDecimal` and returns `bool`. |
| `evalKeyValue` | `(key: string, val: string, target: TargetEnv): bool` | Invokes `evalKeyValue` and returns `bool`. |
| `evalFlag` | `(flag: string, target: TargetEnv): bool` | Invokes `evalFlag` and returns `bool`. |
| `parsePredicate` | `(p: &mut CfgParser, src: string, target: TargetEnv): bool` | Parses input via `parsePredicate` and returns `bool`. |
| `evalCfgExpr` | `(expr: string, target: TargetEnv): CfgEvalResult` | Invokes `evalCfgExpr` and returns `CfgEvalResult`. |

## Minimal Example

```cp
import { parseTargetTriple } from "cargo-platform";

pub fn main() {
    let result = parseTargetTriple("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
