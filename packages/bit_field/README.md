[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# bit_field

Zero-allocation bit and bit-range manipulation utilities for `u8`, `u16`, `u32`, `u64`, `usize`, byte slices, and `BitArray` in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/bit_field](https://crates.io/crates/bit_field) |
| **Upstream Package** | `bit_field` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/phil-opp/rust-bit_field |
| **License** | MIT |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust
use bit_field::{BitField, BitArray};
```

```cp
// CP — same package name, zero-allocation native bit operations
import {
    getBitU64, setBitU64, getBitsU64, setBitsU64,
    getBitsRangeU64, setBitsRangeU64, BitArray,
} from "bit_field";
```

## Installation

```bash
cap add bit_field
```

## Quick Start & Practical Examples

### 1. Single-Bit & Bit-Range Operations on Integers (`u8` / `u16` / `u32` / `u64` / `usize`)

```cp
import {
    getBitU8, setBitU8, getBitsU8, setBitsU8,
    getBitU16, setBitU16, getBitsU16, setBitsU16,
    getBitU32, setBitU32, getBitsU32, setBitsU32,
    getBitU64, setBitU64, getBitsU64, setBitsU64,
    getBitsRangeU64, setBitsRangeU64,
    getBitUsize, setBitUsize, getBitsUsize, setBitsUsize,
} from "bit_field";

fn demoIntegerBits() {
    // Narrow bit-index types (u3 for u8, u4 for u16, u5 for u32, u6 for u64) prevent shift UB
    mut reg: u32 = 0;
    reg = setBitU32(reg, 3, true);          // Set bit 3 -> 0b1000
    let is_set = getBitU32(reg, 3);         // true

    // Extract/insert half-open range [start, end)
    reg = setBitsU32(reg, 8, 16, 0xAB);     // Write 0xAB into bits [8..16)
    let field = getBitsU32(reg, 8, 16);     // 0xAB

    // Full 64-bit range (supports end == 64 via u8 indices)
    mut flags: u64 = 0;
    flags = setBitsRangeU64(flags, 56, 64, 0xFF);
    let top_byte = getBitsRangeU64(flags, 56, 64); // 0xFF
}
```

### 2. Byte-Slice Bit Operations & `BitArray`

```cp
import {
    BitArray, bitLengthSlice,
    getBitSlice, setBitSlice, toggleBitSlice,
    getBitsSlice, setBitsSlice, countOnesSlice,
} from "bit_field";

fn demoBitArray() {
    mut storage: [4]u8 = [0; 4]; // 32 bits on stack
    mut bits = BitArray.fromSlice(&mut storage);

    let total_bits = bits.bitLength(); // 32
    bits.setBit(0, true);
    bits.setBit(15, true);
    bits.toggleBit(0);                 // Flips bit 0 back to false

    // Multi-bit field spanning byte boundaries [6..18)
    bits.setBits(6, 18, 0xABC);
    let extracted = bits.getBits(6, 18); // 0xABC
    let popcount = bits.countOnes();

    bits.setAll();
    bits.clearAll();

    // Free slice helpers work directly on &[]u8 / []u8
    setBitSlice(&mut storage, 10, true);
    let b10 = getBitSlice(&storage, 10); // true
    let ones = countOnesSlice(&storage); // 1
}
```

### 3. Bit-Length & Mask Utilities

```cp
import {
    bitLengthU8, bitLengthU16, bitLengthU32, bitLengthU64, bitLengthUsize,
    bfldMaskU8, bfldMaskU16, bfldMaskU32, bfldMaskU64, bfldFullMaskU64,
} from "bit_field";

fn demoMasks() {
    let w64 = bitLengthU64();   // 64
    let m4 = bfldMaskU8(4);     // 0x0F
    let m64 = bfldFullMaskU64(64); // 0xFFFF_FFFF_FFFF_FFFF
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: All functions operate purely on scalar registers and caller-provided byte slices (`BitArray` borrows `[]u8` in place).
- **Compile-Time Shift Safety**: Integer bit indices use exact bit-width unsigned types (`u3` for `u8`, `u4` for `u16`, `u5` for `u32`, `u6` for `u64`), eliminating invalid shift amounts at the type level.
- **Full-Width 64-Bit Range Support**: `getBitsRangeU64`, `setBitsRangeU64`, and `bfldFullMaskU64` safely handle `end == 64` without 64-bit shift overflow.
- **Cross-Byte Slice Indexing**: `BitArray` and `*Slice` helpers handle arbitrary cross-byte `[start, end)` bit ranges up to 64 bits wide.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
