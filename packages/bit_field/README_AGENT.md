# bit_field — AI Agent Quick Ref

Zero-allocation bit & half-open `[start, end)` bit-range manipulation for `u8`/`u16`/`u32`/`u64`/`usize`, byte slices, and `BitArray` (Rust `bit_field` parity).

## Import

```cp
import {
    bitLengthU8, bitLengthU16, bitLengthU32, bitLengthU64, bitLengthUsize, bitLengthSlice,
    getBitU8, setBitU8, getBitsU8, setBitsU8,
    getBitU16, setBitU16, getBitsU16, setBitsU16,
    getBitU32, setBitU32, getBitsU32, setBitsU32,
    getBitU64, setBitU64, getBitsU64, setBitsU64, getBitsRangeU64, setBitsRangeU64,
    getBitUsize, setBitUsize, getBitsUsize, setBitsUsize,
    getBitSlice, setBitSlice, toggleBitSlice, getBitsSlice, setBitsSlice, countOnesSlice,
    BitArray,
} from "bit_field";
```

## Core API Signatures

```cp
// Integers (half-open [start, end), bit 0 = LSB)
getBitU8(val: u8, bit: u3): bool;     setBitU8(val: u8, bit: u3, on: bool): u8
getBitsU8(val: u8, start: u3, end: u3): u8;   setBitsU8(val: u8, start: u3, end: u3, bits: u8): u8
getBitU16/setBitU16/getBitsU16/setBitsU16 // index type: u4
getBitU32/setBitU32/getBitsU32/setBitsU32 // index type: u5
getBitU64/setBitU64/getBitsU64/setBitsU64 // index type: u6
getBitsRangeU64(val: u64, start: u8, end: u8): u64 // supports end == 64
setBitsRangeU64(val: u64, start: u8, end: u8, bits: u64): u64
getBitUsize/setBitUsize/getBitsUsize/setBitsUsize // index type: u8

// Byte slices (LSB-first across bytes)
bitLengthSlice(bytes: &[]u8): usize
getBitSlice(bytes: &[]u8, bit_index: usize): bool
setBitSlice(bytes: []u8, bit_index: usize, on: bool)
toggleBitSlice(bytes: []u8, bit_index: usize)
getBitsSlice(bytes: &[]u8, start: usize, end: usize): u64
setBitsSlice(bytes: []u8, start: usize, end: usize, value: u64)
countOnesSlice(bytes: &[]u8): usize

// BitArray (borrows mutable []u8)
class BitArray { data: []u8 }
BitArray.fromSlice(buf: []u8): BitArray
ba.bitLength(): usize; ba.getBit(i: usize): bool; ba.setBit(i: usize, on: bool)
ba.toggleBit(i: usize); ba.getBits(start: usize, end: usize): u64
ba.setBits(start: usize, end: usize, value: u64); ba.countOnes(): usize
ba.clearAll(); ba.setAll()
```

## Minimal Example

```cp
import { setBitsRangeU64, getBitsRangeU64, BitArray } from "bit_field";

fn demo(): u64 {
    let x = setBitsRangeU64(0, 8, 16, 0xAB);
    let buf: [4]u8 = [0, 0, 0, 0];
    let mut ba = BitArray.fromSlice(buf[0..4]);
    ba.setBits(4, 12, getBitsRangeU64(x, 8, 16));
    return ba.getBits(4, 12); // 0xAB
}
```

## Gotchas & Borrow Rules

- **Half-open ranges**: All `getBits*`/`setBits*` use `[start, end)` (`end` exclusive).
- **Full 64-bit `u64` range**: `getBitsU64`/`setBitsU64` take `u6` (`0..63`), so `end == 64` cannot fit; use `getBitsRangeU64`/`setBitsRangeU64` (`u8`) instead.
- **Slice mutability**: Read functions take `&[]u8`; `setBitSlice`, `toggleBitSlice`, `setBitsSlice`, and `BitArray.fromSlice` require writable `[]u8`.
- **Mutable `BitArray`**: Declare `let mut ba = BitArray.fromSlice(...)` before calling `setBit`/`toggleBit`/`setBits`/`clearAll`/`setAll`.
