[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bitstream-io

Zero-allocation MSB-first & LSB-first BitReader and BitWriter supporting 0..64-bit reads/writes, peek, byte alignment, and H.264/HEVC Exp-Golomb coding (Rust bitstream-io parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bitstream-io` ([https://github.com/tuffy/bitstream-io](https://github.com/tuffy/bitstream-io))（原库名 `bitstream-io` → CP 包名 `bitstream-io`）

| Metadata | Value |
| --- | --- |
| Package | `bitstream-io` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bitstream-io
```

```cp
import { BitReadResult, BitReader, BitWriter } from "bitstream-io";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BitReadResult` | `class` | Fields: `value: u64, ok: bool` |
| `BitReader` | `class` | Fields: `buf: &[]u8, bit_pos: usize, msb_first: bool` · Methods: `initMsb, initLsb, bitPosition, bitsRemaining, readBitAt, readBit, +5 more` |
| `BitWriter` | `class` | Fields: `buf: []u8, bit_pos: usize, msb_first: bool` · Methods: `initMsb, initLsb, bitsWritten, bytesUsed, writeBit, writeBits, +2 more` |

## Minimal Example

```cp
import { BitReadResult } from "bitstream-io";

pub fn main() {
    // Refer to src/index.d.cp for BitReadResult fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
