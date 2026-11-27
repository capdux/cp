[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# metaphone

Zero-allocation Lawrence Philips Metaphone English phonetic encoding algorithm in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:metaphone` ([https://github.com/words/metaphone](https://github.com/words/metaphone))

| Metadata | Value |
| --- | --- |
| Package | `metaphone` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add metaphone
```

```cp
import { isVowel, metaphone, soundsLike } from "metaphone";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isVowel` | `(b: u8): bool` | Checks predicate `isVowel` and returns `bool`. |
| `metaphone` | `(word: string, max_len: usize, out: []u8): usize` | Invokes `metaphone` and returns `usize`. |
| `soundsLike` | `(a: string, b: string): bool` | Invokes `soundsLike` and returns `bool`. |

## Minimal Example

```cp
import { metaphone } from "metaphone";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = metaphone("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
