[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mathjs

Zero-allocation mathematical expression parser & evaluator (npm mathjs parity) composing libm with scoped variables, constants, 14 built-in functions, and strict error reporting for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mathjs` ([https://github.com/josdejong/mathjs](https://github.com/josdejong/mathjs))（原库名 `mathjs` → CP 包名 `mathjs`）

| Metadata | Value |
| --- | --- |
| Package | `mathjs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mathjs
```

```cp
import {
    ScopeVar,
    ScopeLookup,
    EvalResult,
    MathScope,
    MjParser,
    mjStrEq,
    mjCopyBuf,
    mjIsSpace,
} from "mathjs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ScopeVar` | `class` | Fields: `name: [32]u8, name_len: usize, value: f64` |
| `ScopeLookup` | `class` | Fields: `found: bool, value: f64` |
| `EvalResult` | `class` | Fields: `ok: bool, value: f64, err_code: i32` |
| `MathScope` | `class` | Fields: `vars: [32]ScopeVar, count: usize` · Methods: `new, set, get` |
| `MjParser` | `class` | Fields: `src: string, pos: usize, depth: usize, scope: MathScope` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mjStrEq` | `(a: string, b: string): bool` | Invokes `mjStrEq` and returns `bool`. |
| `mjCopyBuf` | `(dst: []u8, src: string): usize` | Invokes `mjCopyBuf` and returns `usize`. |
| `mjIsSpace` | `(ch: u8): bool` | Invokes `mjIsSpace` and returns `bool`. |
| `mjIsDigit` | `(ch: u8): bool` | Invokes `mjIsDigit` and returns `bool`. |
| `mjIsIdentStart` | `(ch: u8): bool` | Invokes `mjIsIdentStart` and returns `bool`. |
| `mjIsIdentChar` | `(ch: u8): bool` | Invokes `mjIsIdentChar` and returns `bool`. |
| `mjOk` | `(v: f64): EvalResult` | Invokes `mjOk` and returns `EvalResult`. |
| `mjErr` | `(code: i32): EvalResult` | Invokes `mjErr` and returns `EvalResult`. |
| `mjSkipWs` | `(p: &mut MjParser)` | Executes `mjSkipWs` in-place. |
| `mjParseNumber` | `(p: &mut MjParser): EvalResult` | Invokes `mjParseNumber` and returns `EvalResult`. |
| `mjEvalBuiltin` | `(name: string, args: &[]f64, argc: usize): EvalResult` | Invokes `mjEvalBuiltin` and returns `EvalResult`. |
| `mjParsePrimary` | `(p: &mut MjParser): EvalResult` | Invokes `mjParsePrimary` and returns `EvalResult`. |
| `mjParsePower` | `(p: &mut MjParser): EvalResult` | Invokes `mjParsePower` and returns `EvalResult`. |
| `mjParseUnary` | `(p: &mut MjParser): EvalResult` | Invokes `mjParseUnary` and returns `EvalResult`. |
| `mjParseMulDiv` | `(p: &mut MjParser): EvalResult` | Invokes `mjParseMulDiv` and returns `EvalResult`. |
| `mjParseAddSub` | `(p: &mut MjParser): EvalResult` | Invokes `mjParseAddSub` and returns `EvalResult`. |
| `evalExprWithScope` | `(expr: string, scope: MathScope): EvalResult` | Invokes `evalExprWithScope` and returns `EvalResult`. |
| `evalExpr` | `(expr: string): EvalResult` | Invokes `evalExpr` and returns `EvalResult`. |

## Minimal Example

```cp
import { mjStrEq } from "mathjs";

pub fn main() {
    let result = mjStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
