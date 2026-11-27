[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# stemmer

Zero-allocation Martin Porter (1980) English Stemmer (Steps 1a-5b) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:stemmer` ([https://github.com/words/stemmer](https://github.com/words/stemmer)) / `crates.io:rust-stemmers` ([https://github.com/CurrySoftware/rust-stemmers](https://github.com/CurrySoftware/rust-stemmers)) / `crates.io:porter-stemmer` ([https://github.com/jwilm/porter-stemmer](https://github.com/jwilm/porter-stemmer))（原库名 `stemmer` → CP 包名 `stemmer`）

| Metadata | Value |
| --- | --- |
| Package | `stemmer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add stemmer
```

```cp
import {
    isConsonant,
    measurePrefix,
    measure,
    containsVowel,
    endsDoubleConsonant,
    endsCvc,
    tryStep2,
    tryStep3,
} from "stemmer";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isConsonant` | `(buf: &[]u8, idx: usize): bool` | Checks predicate `isConsonant` and returns `bool`. |
| `measurePrefix` | `(buf: &[]u8, end: usize): usize` | Invokes `measurePrefix` and returns `usize`. |
| `measure` | `(word: &[]u8): usize` | Invokes `measure` and returns `usize`. |
| `containsVowel` | `(buf: &[]u8, end: usize): bool` | Invokes `containsVowel` and returns `bool`. |
| `endsDoubleConsonant` | `(buf: &[]u8, end: usize): bool` | Invokes `endsDoubleConsonant` and returns `bool`. |
| `endsCvc` | `(buf: &[]u8, end: usize): bool` | Invokes `endsCvc` and returns `bool`. |
| `tryStep2` | `(buf: []u8, len: usize): usize` | Invokes `tryStep2` and returns `usize`. |
| `tryStep3` | `(buf: []u8, len: usize): usize` | Invokes `tryStep3` and returns `usize`. |
| `tryStep4` | `(buf: []u8, len: usize): usize` | Invokes `tryStep4` and returns `usize`. |
| `stem` | `(word: string, out: []u8): usize` | Invokes `stem` and returns `usize`. |
| `sameStem` | `(a: string, b: string): bool` | Invokes `sameStem` and returns `bool`. |

## Minimal Example

```cp
import { stem } from "stemmer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = stem("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
