[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rand_xorshift

Pure CP zero-allocation Marsaglia Xorshift PRNG family: XorShift128 (rand_xorshift parity), XorShift64Star, and XorShift32

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xorshift` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs)) / `npm:xorshift` ([https://github.com/AndreasMadsen/xorshift](https://github.com/AndreasMadsen/xorshift))（原库名 `rand_xorshift` → CP 包名 `rand_xorshift`）

| Metadata | Value |
| --- | --- |
| Package | `rand_xorshift` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rand_xorshift
```

```cp
import {
    XorShift128,
    XorShift64Star,
    XorShift32,
    xsU64FromHalves,
    xsAddWrap64,
    xsMulWrap64,
    xsSplitMixNext,
    xsReadU32Le,
} from "rand_xorshift";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `XorShift128` | `class` | Fields: `x: u32, y: u32, z: u32, w: u32` · Methods: `fromSeed, fromU64, nextU32, nextU64, nextF64, fillBytes` |
| `XorShift64Star` | `class` | Fields: `state: u64` · Methods: `new, nextU64, nextU32` |
| `XorShift32` | `class` | Fields: `state: u32` · Methods: `new, nextU32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `xsU64FromHalves` | `(hi: u32, lo: u32): u64` | Invokes `xsU64FromHalves` and returns `u64`. |
| `xsAddWrap64` | `(a: u64, b: u64): u64` | Invokes `xsAddWrap64` and returns `u64`. |
| `xsMulWrap64` | `(a: u64, b: u64): u64` | Invokes `xsMulWrap64` and returns `u64`. |
| `xsSplitMixNext` | `(st: &mut u64): u64` | Invokes `xsSplitMixNext` and returns `u64`. |
| `xsReadU32Le` | `(b: [16]u8, off: usize): u32` | Invokes `xsReadU32Le` and returns `u32`. |

## Minimal Example

```cp
import { xsU64FromHalves } from "rand_xorshift";

pub fn main() {
    let result = xsU64FromHalves(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
