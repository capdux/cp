[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cexpr

Zero-allocation C constant expression parser and evaluator (full C operator precedence, hex/bin/oct/char literals) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cexpr` ([https://github.com/jethrogb/rust-cexpr](https://github.com/jethrogb/rust-cexpr))

| Metadata | Value |
| --- | --- |
| Package | `cexpr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cexpr
```

```cp
import {
    EvalResult,
    Parser,
    skipWs,
    peek,
    peek2,
    absI64,
    signedDivI64,
    signedRemI64,
} from "cexpr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EvalResult` | `class` | Fields: `value: i64, ok: bool` |
| `Parser` | `class` | Fields: `pos: usize, ok: bool, depth: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `skipWs` | `(p: &mut Parser, src: string)` | Executes `skipWs` in-place. |
| `peek` | `(p: &mut Parser, src: string): u8` | Invokes `peek` and returns `u8`. |
| `peek2` | `(p: &mut Parser, src: string): u8` | Invokes `peek2` and returns `u8`. |
| `absI64` | `(x: i64): u64` | Invokes `absI64` and returns `u64`. |
| `signedDivI64` | `(a: i64, b: i64): i64` | Invokes `signedDivI64` and returns `i64`. |
| `signedRemI64` | `(a: i64, b: i64): i64` | Invokes `signedRemI64` and returns `i64`. |
| `skipIntSuffix` | `(p: &mut Parser, src: string)` | Executes `skipIntSuffix` in-place. |
| `parseCharLit` | `(p: &mut Parser, src: string): i64` | Parses input via `parseCharLit` and returns `i64`. |
| `parseIntLit` | `(p: &mut Parser, src: string): i64` | Parses input via `parseIntLit` and returns `i64`. |
| `parsePrimary` | `(p: &mut Parser, src: string): i64` | Parses input via `parsePrimary` and returns `i64`. |
| `parseUnary` | `(p: &mut Parser, src: string): i64` | Parses input via `parseUnary` and returns `i64`. |
| `parseMul` | `(p: &mut Parser, src: string): i64` | Parses input via `parseMul` and returns `i64`. |
| `parseAdd` | `(p: &mut Parser, src: string): i64` | Parses input via `parseAdd` and returns `i64`. |
| `parseShift` | `(p: &mut Parser, src: string): i64` | Parses input via `parseShift` and returns `i64`. |
| `parseRel` | `(p: &mut Parser, src: string): i64` | Parses input via `parseRel` and returns `i64`. |
| `parseEq` | `(p: &mut Parser, src: string): i64` | Parses input via `parseEq` and returns `i64`. |
| `parseBitAnd` | `(p: &mut Parser, src: string): i64` | Parses input via `parseBitAnd` and returns `i64`. |
| `parseBitXor` | `(p: &mut Parser, src: string): i64` | Parses input via `parseBitXor` and returns `i64`. |
| `parseBitOr` | `(p: &mut Parser, src: string): i64` | Parses input via `parseBitOr` and returns `i64`. |
| `parseLogAnd` | `(p: &mut Parser, src: string): i64` | Parses input via `parseLogAnd` and returns `i64`. |
| `parseLogOr` | `(p: &mut Parser, src: string): i64` | Parses input via `parseLogOr` and returns `i64`. |
| `parseTernary` | `(p: &mut Parser, src: string): i64` | Parses input via `parseTernary` and returns `i64`. |
| `evalCExpr` | `(expr: string): EvalResult` | Invokes `evalCExpr` and returns `EvalResult`. |

## Minimal Example

```cp
import { parseCharLit } from "cexpr";

pub fn main() {
    let result = parseCharLit(/* p: &mut Parser */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
