[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicase

Zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing (clean-room port of Rust unicase)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicase` ([https://github.com/seanmonstar/unicase](https://github.com/seanmonstar/unicase))

| Metadata | Value |
| --- | --- |
| Package | `unicase` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicase
```

```cp
import {
    Ascii,
    UniCase,
    ucDecodeUtf8,
    ucEncodeUtf8,
    foldAsciiByte,
    foldCodepoint,
    caseFold,
    eqAscii,
} from "unicase";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ascii` | `class` | Fields: `raw: string` · Methods: `new, intoInner, eq, cmp, hash` |
| `UniCase` | `class` | Fields: `raw: string` · Methods: `new, ascii, intoInner, eq, cmp, hash` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ucDecodeUtf8` | `(s: string, pos: usize, step_out: []usize): u32` | Invokes `ucDecodeUtf8` and returns `u32`. |
| `ucEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | Invokes `ucEncodeUtf8` and returns `usize`. |
| `foldAsciiByte` | `(b: u8): u8` | Invokes `foldAsciiByte` and returns `u8`. |
| `foldCodepoint` | `(cp: u32, folded_out: []u32): usize` | Invokes `foldCodepoint` and returns `usize`. |
| `caseFold` | `(src: string, out: []u8): usize` | Invokes `caseFold` and returns `usize`. |
| `eqAscii` | `(a: string, b: string): bool` | Invokes `eqAscii` and returns `bool`. |
| `cmpAscii` | `(a: string, b: string): i32` | Invokes `cmpAscii` and returns `i32`. |
| `cmpUnicode` | `(a: string, b: string): i32` | Invokes `cmpUnicode` and returns `i32`. |
| `eqUnicode` | `(a: string, b: string): bool` | Invokes `eqUnicode` and returns `bool`. |
| `hashAscii` | `(s: string): u64` | Checks predicate `hashAscii` and returns `bool`. |
| `hashUnicode` | `(s: string): u64` | Checks predicate `hashUnicode` and returns `bool`. |
| `startsWithIgnoreCase` | `(haystack: string, prefix: string): bool` | Invokes `startsWithIgnoreCase` and returns `bool`. |
| `endsWithIgnoreCase` | `(haystack: string, suffix: string): bool` | Invokes `endsWithIgnoreCase` and returns `bool`. |
| `containsIgnoreCase` | `(haystack: string, needle: string): bool` | Invokes `containsIgnoreCase` and returns `bool`. |

## Minimal Example

```cp
import { hashAscii } from "unicase";

pub fn main() {
    let result = hashAscii("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
