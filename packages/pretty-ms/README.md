[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pretty-ms

Zero-allocation millisecond decomposer and human-readable duration formatter composing parse-ms for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-ms` ([https://github.com/sindresorhus/pretty-ms](https://github.com/sindresorhus/pretty-ms))

| Metadata | Value |
| --- | --- |
| Package | `pretty-ms` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pretty-ms
```

```cp
import {
    PrettyMsOptions,
    prettyAbsI64,
    pushByte,
    pushStr,
    pushU64,
    pushTwoDigits,
    emitUnit,
    prettyMs,
} from "pretty-ms";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PrettyMsOptions` | `class` | Fields: `verbose: bool, compact: bool, colon_notation: bool, separate_ms: bool, unit_count: u8` · Methods: `defaults` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `prettyAbsI64` | `(v: i64): u64` | Invokes `prettyAbsI64` and returns `u64`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | Mutates or appends state via `pushStr`. |
| `pushU64` | `(out: []u8, pos: usize, val: u64): usize` | Mutates or appends state via `pushU64`. |
| `pushTwoDigits` | `(out: []u8, pos: usize, val: u8): usize` | Mutates or appends state via `pushTwoDigits`. |
| `emitUnit` | `(out: []u8, pos: usize, emitted: u8, val: u64, short_unit: string, singular: string, plural: string, verbose: bool): usize` | Invokes `emitUnit` and returns `usize`. |
| `prettyMs` | `(ms: i64, out: []u8): string` | Invokes `prettyMs` and returns `string`. |
| `prettyMsWith` | `(ms: i64, opts: PrettyMsOptions, out: []u8): string` | Invokes `prettyMsWith` and returns `string`. |

## Minimal Example

```cp
import { pushStr } from "pretty-ms";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
