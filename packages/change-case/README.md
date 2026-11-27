[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# change-case

Zero-heap full-featured case conversion library covering all 12 change-case v5 styles (camel, pascal, snake, kebab, constant, dot, path, train, capital, sentence, no, swap).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:change-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case))

| Metadata | Value |
| --- | --- |
| Package | `change-case` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add change-case
```

```cp
import {
    ccaseIsUpper,
    ccaseIsLower,
    ccaseIsDigit,
    ccaseIsAlnum,
    ccaseToUpper,
    ccaseToLower,
    ccaseJoinLower,
    ccaseCapitalizeWords,
} from "change-case";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ccaseIsUpper` | `(c: u8): bool` | Invokes `ccaseIsUpper` and returns `bool`. |
| `ccaseIsLower` | `(c: u8): bool` | Invokes `ccaseIsLower` and returns `bool`. |
| `ccaseIsDigit` | `(c: u8): bool` | Invokes `ccaseIsDigit` and returns `bool`. |
| `ccaseIsAlnum` | `(c: u8): bool` | Invokes `ccaseIsAlnum` and returns `bool`. |
| `ccaseToUpper` | `(c: u8): u8` | Invokes `ccaseToUpper` and returns `u8`. |
| `ccaseToLower` | `(c: u8): u8` | Invokes `ccaseToLower` and returns `u8`. |
| `ccaseJoinLower` | `(input: string, sep: string, out: []u8): usize` | Invokes `ccaseJoinLower` and returns `usize`. |
| `ccaseCapitalizeWords` | `(out: []u8, len: usize, sep_byte: u8)` | Executes `ccaseCapitalizeWords` in-place. |
| `camelCase` | `(input: string, out: []u8): usize` | Invokes `camelCase` and returns `usize`. |
| `pascalCase` | `(input: string, out: []u8): usize` | Invokes `pascalCase` and returns `usize`. |
| `snakeCase` | `(input: string, out: []u8): usize` | Invokes `snakeCase` and returns `usize`. |
| `kebabCase` | `(input: string, out: []u8): usize` | Invokes `kebabCase` and returns `usize`. |
| `constantCase` | `(input: string, out: []u8): usize` | Invokes `constantCase` and returns `usize`. |
| `dotCase` | `(input: string, out: []u8): usize` | Invokes `dotCase` and returns `usize`. |
| `pathCase` | `(input: string, out: []u8): usize` | Invokes `pathCase` and returns `usize`. |
| `trainCase` | `(input: string, out: []u8): usize` | Invokes `trainCase` and returns `usize`. |
| `capitalCase` | `(input: string, out: []u8): usize` | Invokes `capitalCase` and returns `usize`. |
| `sentenceCase` | `(input: string, out: []u8): usize` | Invokes `sentenceCase` and returns `usize`. |
| `noCase` | `(input: string, out: []u8): usize` | Invokes `noCase` and returns `usize`. |
| `swapCase` | `(input: string, out: []u8): usize` | Invokes `swapCase` and returns `usize`. |

## Minimal Example

```cp
import { ccaseJoinLower } from "change-case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ccaseJoinLower("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
