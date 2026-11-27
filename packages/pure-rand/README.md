[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pure-rand

Zero-allocation pure functional immutable PRNG (Xoroshiro128Plus, SplitMix64 seeding, jump, and unbiased uniformIntDistribution) (clean-room MIT implementation of npm pure-rand, #443 / 124M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pure-rand` ([https://github.com/dubzzz/pure-rand](https://github.com/dubzzz/pure-rand)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `pure-rand` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pure-rand
```

```cp
import {
    Xoroshiro128Plus,
    NextU64Result,
    NextI32Result,
    NextIntRangeResult,
    prRotl64,
    prSplitMix64Step,
    uniformIntDistribution,
} from "pure-rand";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Xoroshiro128Plus` | `class` | Fields: `s0: u64, s1: u64` · Methods: `fromSeed, fromState, nextU64, nextI32, jump` |
| `NextU64Result` | `class` | Fields: `value: u64, next: Xoroshiro128Plus` |
| `NextI32Result` | `class` | Fields: `value: i32, next: Xoroshiro128Plus` |
| `NextIntRangeResult` | `class` | Fields: `value: i64, next: Xoroshiro128Plus` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `prRotl64` | `(x: u64, k: u6): u64` | Invokes `prRotl64` and returns `u64`. |
| `prSplitMix64Step` | `(state: u64): u64` | Invokes `prSplitMix64Step` and returns `u64`. |
| `uniformIntDistribution` | `(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult` | Invokes `uniformIntDistribution` and returns `NextIntRangeResult`. |

## Minimal Example

```cp
import { prRotl64 } from "pure-rand";

pub fn main() {
    let result = prRotl64(0, /* k: u6 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
