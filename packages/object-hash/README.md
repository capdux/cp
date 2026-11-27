[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# object-hash

Zero-allocation deterministic order-independent structured object hasher using FNV-1a 64-bit (npm object-hash parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`object-hash`](https://github.com/puleos/object-hash)（`MIT`）（包名 `object-hash`）。

| Metadata | Value |
| --- | --- |
| Package | `object-hash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add object-hash
```

```cp
import {
    HashField,
    ohshFnvOffset,
    ohshFnvPrime,
    ohshFeedByte,
    ohshFeedBytes,
    ohshCmpStr,
    ohshFieldLess,
    ohshHexDigit,
} from "object-hash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HashField` | `class` | Fields: `key: string, value: string, kind: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ohshFnvOffset` | `(): u64` | Invokes `ohshFnvOffset` and returns `u64`. |
| `ohshFnvPrime` | `(): u64` | Invokes `ohshFnvPrime` and returns `u64`. |
| `ohshFeedByte` | `(h: u64, b: u8): u64` | Invokes `ohshFeedByte` and returns `u64`. |
| `ohshFeedBytes` | `(h: u64, data: &[]u8): u64` | Invokes `ohshFeedBytes` and returns `u64`. |
| `ohshCmpStr` | `(a: string, b: string): i32` | Invokes `ohshCmpStr` and returns `i32`. |
| `ohshFieldLess` | `(a: HashField, b: HashField): bool` | Invokes `ohshFieldLess` and returns `bool`. |
| `ohshHexDigit` | `(nibble: u8): u8` | Invokes `ohshHexDigit` and returns `u8`. |
| `hashBytesFnv64` | `(data: &[]u8): u64` | Checks predicate `hashBytesFnv64` and returns `bool`. |
| `hashStringFnv64` | `(s: string): u64` | Checks predicate `hashStringFnv64` and returns `bool`. |
| `hashFieldsFnv64` | `(fields: &[]HashField): u64` | Checks predicate `hashFieldsFnv64` and returns `bool`. |
| `hashFieldsHex` | `(fields: &[]HashField, out: []u8): usize` | Checks predicate `hashFieldsHex` and returns `bool`. |

## Minimal Example

```cp
import { hashStringFnv64 } from "object-hash";

pub fn main() {
    let result = hashStringFnv64("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
