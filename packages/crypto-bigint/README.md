[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# crypto-bigint

Zero-allocation constant-time fixed-width 256-bit unsigned integer (U256) arithmetic and modular operations (RustCrypto crypto-bigint parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crypto-bigint` ([https://github.com/RustCrypto/crypto-bigint](https://github.com/RustCrypto/crypto-bigint))

| Metadata | Value |
| --- | --- |
| Package | `crypto-bigint` |
| Version | `1.0.0` |
| License | `Apache-2.0 OR MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add crypto-bigint
```

```cp
import {
    CbiAddRes,
    CbiSubRes,
    U256,
    cbiMask64,
    cbiAllOnes64,
    cbiCtMask,
    cbiReadBeU64,
    cbiReadLeU64,
} from "crypto-bigint";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CbiAddRes` | `class` | Fields: `val: U256, carry: u64` |
| `CbiSubRes` | `class` | Fields: `val: U256, borrow: u64` |
| `U256` | `class` | Fields: `limbs: [4]u64` · Methods: `fromU64, fromBeBytes, fromLeBytes, toBeBytes, toLeBytes, fromBeHex, +12 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cbiMask64` | `(): u128` | Invokes `cbiMask64` and returns `u128`. |
| `cbiAllOnes64` | `(): u64` | Invokes `cbiAllOnes64` and returns `u64`. |
| `cbiCtMask` | `(cond: bool): u64` | Invokes `cbiCtMask` and returns `u64`. |
| `cbiReadBeU64` | `(b: &[]u8, off: usize): u64` | Invokes `cbiReadBeU64` and returns `u64`. |
| `cbiReadLeU64` | `(b: &[]u8, off: usize): u64` | Invokes `cbiReadLeU64` and returns `u64`. |
| `cbiWriteBeU64` | `(v: u64, out: []u8, off: usize)` | Executes `cbiWriteBeU64` in-place. |
| `cbiWriteLeU64` | `(v: u64, out: []u8, off: usize)` | Executes `cbiWriteLeU64` in-place. |
| `cbiHexNibble` | `(ch: u8): i32` | Invokes `cbiHexNibble` and returns `i32`. |
| `cbiHexChar` | `(nibble: u8): u8` | Invokes `cbiHexChar` and returns `u8`. |
| `cbiBitLen64` | `(v_in: u64): usize` | Invokes `cbiBitLen64` and returns `usize`. |
| `cbiAddWithCarry` | `(a: U256, b: U256): CbiAddRes` | Invokes `cbiAddWithCarry` and returns `CbiAddRes`. |
| `cbiSubWithBorrow` | `(a: U256, b: U256): CbiSubRes` | Invokes `cbiSubWithBorrow` and returns `CbiSubRes`. |
| `addMod` | `(a: U256, b: U256, m: U256): U256` | Mutates or appends state via `addMod`. |
| `subMod` | `(a: U256, b: U256, m: U256): U256` | Invokes `subMod` and returns `U256`. |

## Minimal Example

```cp
import { cbiMask64 } from "crypto-bigint";

pub fn main() {
    let result = cbiMask64();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
