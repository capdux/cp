[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unique-names-generator

Zero-allocation deterministic human-readable name generator for containers, sandboxes & subdomains (unique-names-generator / human-id / Rust petname parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:unique-names-generator` ([https://github.com/andreasonny83/unique-names-generator](https://github.com/andreasonny83/unique-names-generator))（原库名 `unique-names-generator` → CP 包名 `unique-names-generator`）

| Metadata | Value |
| --- | --- |
| Package | `unique-names-generator` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unique-names-generator
```

```cp
import {
    Petname,
    splitmix64Next,
    adjectiveAt,
    adverbAt,
    nounAt,
    appendStr,
    appendHex4,
    petnameFromSeed,
} from "unique-names-generator";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Petname` | `class` | Fields: `state: u64` · Methods: `init, nextU64, generate, generateSlug, generateSubdomain` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `splitmix64Next` | `(state: u64): u64` | Invokes `splitmix64Next` and returns `u64`. |
| `adjectiveAt` | `(idx: usize): string` | Invokes `adjectiveAt` and returns `string`. |
| `adverbAt` | `(idx: usize): string` | Invokes `adverbAt` and returns `string`. |
| `nounAt` | `(idx: usize): string` | Invokes `nounAt` and returns `string`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendHex4` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `appendHex4` and returns `usize`. |
| `petnameFromSeed` | `(seed: u64, words: u8, sep: string, out: []u8): string` | Invokes `petnameFromSeed` and returns `string`. |
| `subdomainFromSeed` | `(seed: u64, out: []u8): string` | Invokes `subdomainFromSeed` and returns `string`. |

## Minimal Example

```cp
import { appendStr } from "unique-names-generator";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
