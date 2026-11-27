[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse-json

Zero-allocation RFC 8259 JSON diagnostic validator with line/column locations and codeFrame rendering (npm parse-json v8 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-json` ([https://github.com/sindresorhus/parse-json](https://github.com/sindresorhus/parse-json))（原库名 `parse-json` → CP 包名 `parse-json`）

| Metadata | Value |
| --- | --- |
| Package | `parse-json` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parse-json
```

```cp
import {
    JsonDiagKind,
    JsonDiagResult,
    PjsParser,
    PjsLoc,
    PjsLineSpan,
    PjsWriter,
    pjsIsWs,
    pjsIsDigit,
} from "parse-json";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `JsonDiagKind` | `Ok, EmptyInput, UnexpectedToken, UnexpectedEnd, TrailingComma, SingleQuotedString, UnquotedKey, LeadingZero ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JsonDiagResult` | `class` | Fields: `ok: bool, kind: JsonDiagKind, offset: usize, line: usize, column: usize, value_kind: u8` |
| `PjsParser` | `class` | Fields: `input: string, pos: usize, has_err: bool, err_kind: JsonDiagKind, err_offset: usize` |
| `PjsLoc` | `class` | Fields: `line: usize, column: usize` |
| `PjsLineSpan` | `class` | Fields: `found: bool, start: usize, end: usize` |
| `PjsWriter` | `class` | Fields: `buf: []u8, pos: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pjsIsWs` | `(ch: u8): bool` | Invokes `pjsIsWs` and returns `bool`. |
| `pjsIsDigit` | `(ch: u8): bool` | Invokes `pjsIsDigit` and returns `bool`. |
| `pjsIsHex` | `(ch: u8): bool` | Invokes `pjsIsHex` and returns `bool`. |
| `pjsIsIdentStart` | `(ch: u8): bool` | Invokes `pjsIsIdentStart` and returns `bool`. |
| `pjsSkipWs` | `(p: &mut PjsParser)` | Executes `pjsSkipWs` in-place. |
| `pjsSetErr` | `(p: &mut PjsParser, kind: JsonDiagKind, offset: usize)` | Executes `pjsSetErr` in-place. |
| `pjsLineCol` | `(input: string, offset: usize): PjsLoc` | Invokes `pjsLineCol` and returns `PjsLoc`. |
| `pjsParseString` | `(p: &mut PjsParser)` | Executes `pjsParseString` in-place. |
| `pjsParseNumber` | `(p: &mut PjsParser)` | Executes `pjsParseNumber` in-place. |
| `pjsExpectKeyword` | `(p: &mut PjsParser, kw: string)` | Executes `pjsExpectKeyword` in-place. |
| `pjsParseValue` | `(p: &mut PjsParser, depth: usize): u8` | Invokes `pjsParseValue` and returns `u8`. |
| `validateJsonWithDiag` | `(input: string): JsonDiagResult` | Validates input with `validateJsonWithDiag` returning `JsonDiagResult`. |
| `pjsDiagMessage` | `(kind: JsonDiagKind): string` | Invokes `pjsDiagMessage` and returns `string`. |
| `pjsWriteByte` | `(w: &mut PjsWriter, b: u8)` | Executes `pjsWriteByte` in-place. |
| `pjsWriteStr` | `(w: &mut PjsWriter, s: string)` | Executes `pjsWriteStr` in-place. |
| `pjsNumWidth` | `(n: usize): usize` | Invokes `pjsNumWidth` and returns `usize`. |
| `pjsWriteUsize` | `(w: &mut PjsWriter, n: usize)` | Executes `pjsWriteUsize` in-place. |
| `pjsWriteSpaces` | `(w: &mut PjsWriter, count: usize)` | Executes `pjsWriteSpaces` in-place. |
| `pjsFindLineSpan` | `(input: string, target_line: usize): PjsLineSpan` | Invokes `pjsFindLineSpan` and returns `PjsLineSpan`. |
| `pjsWriteFrameLine` | `(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool)` | Executes `pjsWriteFrameLine` in-place. |
| `formatJsonError` | `(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize` | Serializes/formats data via `formatJsonError` returning `usize`. |

## Minimal Example

```cp
import { validateJsonWithDiag } from "parse-json";

pub fn main() {
    let result = validateJsonWithDiag("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
