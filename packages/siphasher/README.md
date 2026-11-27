[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# siphasher

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`siphasher`](https://github.com/jedisct1/rust-siphash)（`MIT OR Apache-2.0`）（原库名 `siphasher`，CP 包名 `siphasher`）。

| Metadata | Value |
| --- | --- |
| Package | `siphasher` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add siphasher
```

```cp
import {
    SipState,
    SipHasher24,
    SipHasher13,
    addWrap64,
    rotl64,
    sipRound,
    loadLe64,
    loadLe64Str,
} from "siphasher";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SipState` | `class` | Fields: `v0: u64, v1: u64, v2: u64, v3: u64` |
| `SipHasher24` | `class` | Fields: `v0: u64, v1: u64, v2: u64, v3: u64, buf: [8]u8, buf_len: usize, +1 more` · Methods: `init, compress, write, finish` |
| `SipHasher13` | `class` | Fields: `v0: u64, v1: u64, v2: u64, v3: u64, buf: [8]u8, buf_len: usize, +1 more` · Methods: `init, compress, write, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `rotl64` | `(x: u64, r: u64): u64` | Invokes `rotl64` and returns `u64`. |
| `sipRound` | `(s: SipState): SipState` | Invokes `sipRound` and returns `SipState`. |
| `loadLe64` | `(buf: []u8, offset: usize): u64` | Invokes `loadLe64` and returns `u64`. |
| `loadLe64Str` | `(s: string, offset: usize): u64` | Invokes `loadLe64Str` and returns `u64`. |
| `siphash24` | `(k0: u64, k1: u64, data: string): u64` | Invokes `siphash24` and returns `u64`. |
| `siphash13` | `(k0: u64, k1: u64, data: string): u64` | Invokes `siphash13` and returns `u64`. |

## Minimal Example

```cp
import { loadLe64Str } from "siphasher";

pub fn main() {
    let result = loadLe64Str("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
