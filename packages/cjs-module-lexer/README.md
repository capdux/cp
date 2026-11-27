[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cjs-module-lexer

Zero-allocation CommonJS static named export and re-export lexer (npm cjs-module-lexer parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cjs-module-lexer` ([https://github.com/nodejs/cjs-module-lexer](https://github.com/nodejs/cjs-module-lexer))（原库名 `cjs-module-lexer` → CP 包名 `cjs-module-lexer`）

| Metadata | Value |
| --- | --- |
| Package | `cjs-module-lexer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cjs-module-lexer
```

```cp
import {
    CjsParseResult,
    CjslLexer,
    CjslStrRes,
    cjslIsWs,
    cjslIsIdentStart,
    cjslIsIdentChar,
    cjslStrEq,
    cjslAddExport,
} from "cjs-module-lexer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CjsParseResult` | `class` | Fields: `exports: [32]string, export_count: usize, reexports: [16]string, reexport_count: usize` |
| `CjslLexer` | `class` | Fields: `src: string, pos: usize` |
| `CjslStrRes` | `class` | Fields: `ok: bool, val: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cjslIsWs` | `(ch: u8): bool` | Invokes `cjslIsWs` and returns `bool`. |
| `cjslIsIdentStart` | `(ch: u8): bool` | Invokes `cjslIsIdentStart` and returns `bool`. |
| `cjslIsIdentChar` | `(ch: u8): bool` | Invokes `cjslIsIdentChar` and returns `bool`. |
| `cjslStrEq` | `(a: string, b: string): bool` | Invokes `cjslStrEq` and returns `bool`. |
| `cjslAddExport` | `(res: &mut CjsParseResult, name: string)` | Executes `cjslAddExport` in-place. |
| `cjslAddReexport` | `(res: &mut CjsParseResult, spec: string)` | Executes `cjslAddReexport` in-place. |
| `cjslSkipString` | `(lex: &mut CjslLexer)` | Executes `cjslSkipString` in-place. |
| `cjslSkipWsAndComments` | `(lex: &mut CjslLexer)` | Executes `cjslSkipWsAndComments` in-place. |
| `cjslReadIdent` | `(lex: &mut CjslLexer): string` | Invokes `cjslReadIdent` and returns `string`. |
| `cjslReadStringLit` | `(lex: &mut CjslLexer): CjslStrRes` | Invokes `cjslReadStringLit` and returns `CjslStrRes`. |
| `cjslMatchRequireCall` | `(lex: &mut CjslLexer): CjslStrRes` | Invokes `cjslMatchRequireCall` and returns `CjslStrRes`. |
| `cjslSkipPropValue` | `(lex: &mut CjslLexer)` | Executes `cjslSkipPropValue` in-place. |
| `cjslParseObjectExports` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | Executes `cjslParseObjectExports` in-place. |
| `cjslParseExportsMemberAssign` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | Executes `cjslParseExportsMemberAssign` in-place. |
| `cjslParseDefineProperty` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | Executes `cjslParseDefineProperty` in-place. |
| `cjslParseExportStar` | `(lex: &mut CjslLexer, res: &mut CjsParseResult)` | Executes `cjslParseExportStar` in-place. |
| `parseCjs` | `(src: string): CjsParseResult` | Parses input via `parseCjs` and returns `CjsParseResult`. |

## Minimal Example

```cp
import { parseCjs } from "cjs-module-lexer";

pub fn main() {
    let result = parseCjs("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
