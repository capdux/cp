[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bit_field

Zero-allocation bit and bit-range manipulation library for CP (`u8`, `u16`, `u32`, `u64`, `usize`, and arbitrary byte slices / `BitArray`), modeled on Rust's [`bit_field`](https://github.com/phil-opp/rust-bit_field) crate (MIT, CP v1.0.0).

Supports single-bit queries/mutations, half-open `[start, end)` bit-field extraction and insertion, cross-byte LSB-first slice bitfields (up to 64 bits at once), population counts (`countOnesSlice`), and a zero-heap `BitArray` wrapper over caller-owned `[]u8` buffers.

## Installation & Import

```bash
cap add bit_field
```

```cp
import {
    getBitU64,
    setBitU64,
    getBitsRangeU64,
    setBitsRangeU64,
    getBitsU32,
    setBitsU32,
    getBitsSlice,
    setBitsSlice,
    BitArray,
} from "bit_field";
```

## Core API Reference

### Integer Bit & Bit-Range Functions

All bit ranges use half-open `[start, end)` bounds (bit `0` is LSB):

| Type | Functions | Index Width |
| :--- | :--- | :--- |
| `u8` | `bitLengthU8()`, `getBitU8`, `setBitU8`, `getBitsU8`, `setBitsU8`, `bfldMaskU8` | `u3` (`0..7`) |
| `u16` | `bitLengthU16()`, `getBitU16`, `setBitU16`, `getBitsU16`, `setBitsU16`, `bfldMaskU16` | `u4` (`0..15`) |
| `u32` | `bitLengthU32()`, `getBitU32`, `setBitU32`, `getBitsU32`, `setBitsU32`, `bfldMaskU32` | `u5` (`0..31`) |
| `u64` | `bitLengthU64()`, `getBitU64`, `setBitU64`, `getBitsU64`, `setBitsU64`, `bfldMaskU64` | `u6` (`0..63`) |
| `u64` (full-width) | `getBitsRangeU64(val, start: u8, end: u8): u64`, `setBitsRangeU64(val, start: u8, end: u8, bits: u64): u64`, `bfldFullMaskU64(width: u8): u64` | `u8` (`end` up to `64`) |
| `usize` | `bitLengthUsize()`, `getBitUsize`, `setBitUsize`, `getBitsUsize`, `setBitsUsize` | `u8` |

### Byte Slice & `BitArray` API

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `bitLengthSlice` | `(bytes: &[]u8): usize` | Returns total bit capacity (`bytes.len * 8`). |
| `getBitSlice` | `(bytes: &[]u8, bit_index: usize): bool` | Reads bit at `bit_index` (LSB-first within each byte). |
| `setBitSlice` | `(bytes: []u8, bit_index: usize, on: bool)` | Sets or clears bit at `bit_index` in place. |
| `toggleBitSlice` | `(bytes: []u8, bit_index: usize)` | Flips bit at `bit_index` in place. |
| `getBitsSlice` | `(bytes: &[]u8, start: usize, end: usize): u64` | Extracts `[start, end)` bits (up to 64 bits across byte boundaries). |
| `setBitsSlice` | `(bytes: []u8, start: usize, end: usize, value: u64)` | Writes `[start, end)` bits into `bytes` in place. |
| `countOnesSlice` | `(bytes: &[]u8): usize` | Counts total set (`1`) bits across the byte slice. |
| `BitArray` | `fromSlice(buf: []u8)`, `bitLength()`, `getBit(i)`, `setBit(i, on)`, `toggleBit(i)`, `getBits(s, e)`, `setBits(s, e, val)`, `countOnes()`, `clearAll()`, `setAll()` | Zero-heap object-oriented wrapper over a mutable `[]u8` buffer. |

## Usage Examples

### 1. Hardware Register / Page-Table Entry Manipulation (`u64`)

```cp
import { setBitU64, setBitsRangeU64, getBitsRangeU64, getBitU64 } from "bit_field";

fn buildPageEntry(phys_frame: u64): u64 {
    let mut entry: u64 = 0;
    entry = setBitU64(entry, 0, true);                 // Present bit
    entry = setBitU64(entry, 1, true);                 // Writable bit
    entry = setBitsRangeU64(entry, 12, 52, phys_frame); // 40-bit frame address
    let present = getBitU64(entry, 0);                 // true
    let frame = getBitsRangeU64(entry, 12, 52);        // phys_frame
    return if (present) { frame } else { 0 };
}
```

### 2. Cross-Byte Packed Protocol Fields (`BitArray`)

```cp
import { BitArray } from "bit_field";

fn bitArrayExample(): usize {
    let buf: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    let mut bits = BitArray.fromSlice(buf[0..8]);
    bits.setBit(3, true);
    bits.setBits(5, 17, 0xABC); // 12-bit value spanning bytes 0..2
    let v = bits.getBits(5, 17); // 0xABC
    bits.toggleBit(3);
    return bits.countOnes();
}
```

## Differences from Upstream & Safety Highlights

- **Exact Shift-Width Types**: Fixed-width helpers (`getBitU8`..`getBitU64`) use CP's narrow unsigned shift types (`u3`/`u4`/`u5`/`u6`) so bit indices cannot exceed the integer's width at compile time; use `getBitsRangeU64` / `setBitsRangeU64` when `end == 64`.
- **Pure Functional Integer Ops + In-Place Slices**: Integer operations return updated values without side effects; `BitArray` and `*Slice` mutators edit caller-owned `[]u8` in place with zero heap allocation.
- **Bounds-Safe Slice Access**: Out-of-bounds slice bit operations safely clamp or no-op rather than panicking.
