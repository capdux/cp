[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fast-deep-equal

Zero-heap semantic JSON deep equality checker and root type classifier in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-deep-equal` ([https://github.com/epoberezkin/fast-deep-equal](https://github.com/epoberezkin/fast-deep-equal))

| Metadata | Value |
| --- | --- |
| Package | `fast-deep-equal` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fast-deep-equal
```

```cp
import {
    ValueSpan,
    DecChar,
    isWs,
    skipWs,
    matchLit,
    hexDigit,
    invalidSpan,
    scanString,
} from "fast-deep-equal";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ValueSpan` | `class` | Fields: `start: usize, end: usize, kind: u8, valid: bool` |
| `DecChar` | `class` | Fields: `code: u32, next_pos: usize, done: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isWs` | `(ch: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `skipWs` | `(s: string, pos: usize): usize` | Invokes `skipWs` and returns `usize`. |
| `matchLit` | `(s: string, pos: usize, lit: string): bool` | Invokes `matchLit` and returns `bool`. |
| `hexDigit` | `(ch: u8): i32` | Invokes `hexDigit` and returns `i32`. |
| `invalidSpan` | `(): ValueSpan` | Invokes `invalidSpan` and returns `ValueSpan`. |
| `scanString` | `(s: string, pos: usize): ValueSpan` | Invokes `scanString` and returns `ValueSpan`. |
| `scanNumber` | `(s: string, pos: usize): ValueSpan` | Invokes `scanNumber` and returns `ValueSpan`. |
| `parseValueSpan` | `(s: string, pos: usize, depth: u32): ValueSpan` | Parses input via `parseValueSpan` and returns `ValueSpan`. |
| `kindOfJson` | `(s: string): u8` | Invokes `kindOfJson` and returns `u8`. |
| `isPlainObjectJson` | `(s: string): bool` | Checks predicate `isPlainObjectJson` and returns `bool`. |
| `parseJsonNumberF64` | `(s: string): f64` | Parses input via `parseJsonNumberF64` and returns `f64`. |
| `nextDecodedChar` | `(s: string, pos: usize): DecChar` | Invokes `nextDecodedChar` and returns `DecChar`. |
| `jsonStringsEqual` | `(a: string, b: string): bool` | Invokes `jsonStringsEqual` and returns `bool`. |
| `countObjectMembers` | `(obj: string): usize` | Invokes `countObjectMembers` and returns `usize`. |
| `arraysDeepEqual` | `(a: string, b: string, depth: u32): bool` | Invokes `arraysDeepEqual` and returns `bool`. |
| `objectsDeepEqual` | `(a: string, b: string, depth: u32): bool` | Invokes `objectsDeepEqual` and returns `bool`. |
| `valuesDeepEqual` | `(a: string, b: string, kind: u8, depth: u32): bool` | Invokes `valuesDeepEqual` and returns `bool`. |
| `jsonDeepEqual` | `(a: string, b: string): bool` | Invokes `jsonDeepEqual` and returns `bool`. |

## Minimal Example

```cp
import { parseValueSpan } from "fast-deep-equal";

pub fn main() {
    let result = parseValueSpan("example", 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
