[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# regex-escape

Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:regex-escape` ([https://github.com/rust-lang/regex](https://github.com/rust-lang/regex))

| Metadata | Value |
| --- | --- |
| Package | `regex-escape` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add regex-escape
```

```cp
import {
    isMetaChar,
    isEscapeTarget,
    needsEscape,
    escapedLen,
    escapeRegex,
    escapeCharClass,
    unescapeRegexLiteral,
} from "regex-escape";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isMetaChar` | `(c: u8): bool` | Checks predicate `isMetaChar` and returns `bool`. |
| `isEscapeTarget` | `(c: u8): bool` | Checks predicate `isEscapeTarget` and returns `bool`. |
| `needsEscape` | `(input: string): bool` | Invokes `needsEscape` and returns `bool`. |
| `escapedLen` | `(input: string): usize` | Invokes `escapedLen` and returns `usize`. |
| `escapeRegex` | `(input: string, out: []u8): usize` | Invokes `escapeRegex` and returns `usize`. |
| `escapeCharClass` | `(input: string, out: []u8): usize` | Invokes `escapeCharClass` and returns `usize`. |
| `unescapeRegexLiteral` | `(input: string, out: []u8): usize` | Invokes `unescapeRegexLiteral` and returns `usize`. |

## Minimal Example

```cp
import { needsEscape } from "regex-escape";

pub fn main() {
    let result = needsEscape("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
