[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# js-tokens

Zero-allocation tiny JavaScript/TypeScript lexer tokenizer (clean-room MIT implementation of npm js-tokens, #61 / 381M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:js-tokens` ([https://github.com/lydell/js-tokens](https://github.com/lydell/js-tokens)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `js-tokens` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add js-tokens
```

```cp
import {
    JsTokenKind,
    JsToken,
    jstIsIdentStart,
    jstIsIdentPart,
    jstIsDigit,
    jstIsHex,
    jstIsSpace,
    jstStartsWith,
} from "js-tokens";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `JsTokenKind` | `IdentifierName, NumericLiteral, StringLiteral, NoSubstitutionTemplate, TemplateHead, Punctuator, SingleLineComment, MultiLineComment ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JsToken` | `class` | Fields: `kind: JsTokenKind, start: usize, end: usize, closed: bool` · Methods: `text` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jstIsIdentStart` | `(b: u8): bool` | Invokes `jstIsIdentStart` and returns `bool`. |
| `jstIsIdentPart` | `(b: u8): bool` | Invokes `jstIsIdentPart` and returns `bool`. |
| `jstIsDigit` | `(b: u8): bool` | Invokes `jstIsDigit` and returns `bool`. |
| `jstIsHex` | `(b: u8): bool` | Invokes `jstIsHex` and returns `bool`. |
| `jstIsSpace` | `(b: u8): bool` | Invokes `jstIsSpace` and returns `bool`. |
| `jstStartsWith` | `(src: string, pos: usize, lit: string): bool` | Invokes `jstStartsWith` and returns `bool`. |
| `nextJsToken` | `(src: string, pos: usize): JsToken?` | Invokes `nextJsToken` and returns `JsToken?`. |
| `tokenizeJs` | `(src: string, out: []JsToken): usize` | Invokes `tokenizeJs` and returns `usize`. |
| `countJsTokens` | `(src: string): usize` | Invokes `countJsTokens` and returns `usize`. |

## Minimal Example

```cp
import { jstStartsWith } from "js-tokens";

pub fn main() {
    let result = jstStartsWith("example", 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
