[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hashbrown

Zero-heap SwissTable-style control-byte hash map and hash set (clean-room CP implementation of crates.io #1 hashbrown)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hashbrown` ([https://github.com/rust-lang/hashbrown](https://github.com/rust-lang/hashbrown))

| Metadata | Value |
| --- | --- |
| Package | `hashbrown` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hashbrown
```

```cp
import {
    HbGetResult,
    HbInsertResult,
    HashMapU64,
    HashSetU64,
    hbHashU64,
    hbH2,
} from "hashbrown";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HbGetResult` | `class` | Fields: `found: bool, value: i64` |
| `HbInsertResult` | `class` | Fields: `ok: bool, replaced: bool, old_value: i64` |
| `HashMapU64` | `class` | Fields: `ctrl: [128]u8, keys: [128]u64, vals: [128]i64, used: usize` · Methods: `new, len, capacity, isEmpty, clear, findSlot, +4 more` |
| `HashSetU64` | `class` | Fields: `inner: HashMapU64` · Methods: `new, len, isEmpty, clear, insert, contains, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hbHashU64` | `(key: u64): u64` | Invokes `hbHashU64` and returns `u64`. |
| `hbH2` | `(h: u64): u8` | Invokes `hbH2` and returns `u8`. |

## Minimal Example

```cp
import { hbHashU64 } from "hashbrown";

pub fn main() {
    let result = hbHashU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
