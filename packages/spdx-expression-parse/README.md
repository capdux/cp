[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# spdx-expression-parse

Zero-allocation SPDX license expression recursive-descent AST parser and evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:spdx-expression-parse` ([https://github.com/jslicense/spdx-expression-parse.js](https://github.com/jslicense/spdx-expression-parse.js))

| Metadata | Value |
| --- | --- |
| Package | `spdx-expression-parse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add spdx-expression-parse
```

```cp
import {
    SpdxAstNode,
    SpdxAst,
    SpdxpState,
    spdxpEmptyNode,
    spdxpEmptyAst,
    spdxpIsIdChar,
    spdxpIsKeyword,
    spdxpIsAllIdChars,
} from "spdx-expression-parse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SpdxAstNode` | `class` | Fields: `kind: u8, license: string, exception: string, plus: bool, left: i32, right: i32` |
| `SpdxAst` | `class` | Fields: `nodes: [32]SpdxAstNode, root: i32, node_count: usize, valid: bool` |
| `SpdxpState` | `class` | Fields: `src: string, pos: usize, depth: usize, failed: bool, ast: SpdxAst` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `spdxpEmptyNode` | `(): SpdxAstNode` | Invokes `spdxpEmptyNode` and returns `SpdxAstNode`. |
| `spdxpEmptyAst` | `(): SpdxAst` | Invokes `spdxpEmptyAst` and returns `SpdxAst`. |
| `spdxpIsIdChar` | `(ch: u8): bool` | Invokes `spdxpIsIdChar` and returns `bool`. |
| `spdxpIsKeyword` | `(tok: string): bool` | Invokes `spdxpIsKeyword` and returns `bool`. |
| `spdxpIsAllIdChars` | `(s: string): bool` | Invokes `spdxpIsAllIdChars` and returns `bool`. |
| `spdxpIsValidLicenseRef` | `(id: string): bool` | Invokes `spdxpIsValidLicenseRef` and returns `bool`. |
| `spdxpIsValidLicenseId` | `(id: string): bool` | Invokes `spdxpIsValidLicenseId` and returns `bool`. |
| `spdxpIsValidExceptionId` | `(exc: string): bool` | Invokes `spdxpIsValidExceptionId` and returns `bool`. |
| `spdxpSkipSpaces` | `(st: &mut SpdxpState)` | Executes `spdxpSkipSpaces` in-place. |
| `spdxpAllocNode` | `(st: &mut SpdxpState, node: SpdxAstNode): i32` | Invokes `spdxpAllocNode` and returns `i32`. |
| `spdxpConsumeKeyword` | `(st: &mut SpdxpState, kw: string): bool` | Invokes `spdxpConsumeKeyword` and returns `bool`. |
| `spdxpParseSimple` | `(st: &mut SpdxpState): i32` | Invokes `spdxpParseSimple` and returns `i32`. |
| `spdxpParsePrimary` | `(st: &mut SpdxpState): i32` | Invokes `spdxpParsePrimary` and returns `i32`. |
| `spdxpParseAnd` | `(st: &mut SpdxpState): i32` | Invokes `spdxpParseAnd` and returns `i32`. |
| `spdxpParseOr` | `(st: &mut SpdxpState): i32` | Invokes `spdxpParseOr` and returns `i32`. |
| `parseSpdxExpression` | `(expr: string): SpdxAst` | Parses input via `parseSpdxExpression` and returns `SpdxAst`. |
| `isValidSpdxExpression` | `(expr: string): bool` | Checks predicate `isValidSpdxExpression` and returns `bool`. |
| `spdxpMatchLicenseNode` | `(node: SpdxAstNode, allowed: string): bool` | Invokes `spdxpMatchLicenseNode` and returns `bool`. |
| `spdxpEvalAst` | `(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool` | Invokes `spdxpEvalAst` and returns `bool`. |
| `satisfiesSpdx` | `(ast: SpdxAst, allowed_license: string): bool` | Invokes `satisfiesSpdx` and returns `bool`. |

## Minimal Example

```cp
import { parseSpdxExpression } from "spdx-expression-parse";

pub fn main() {
    let result = parseSpdxExpression("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
