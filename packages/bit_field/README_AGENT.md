# bit_field — AI Agent Quick Ref

- **Import**: `import { ... } from "bit_field";`
- **Origin**: crates.io `bit_field` (v1.0.0, MIT, zero-alloc)
- **Full Types**: `src/index.d.cp`

## Bit-Length & Mask Helpers
- `bitLengthU8(): usize` | `bitLengthU16()` | `bitLengthU32()` | `bitLengthU64()` | `bitLengthUsize()`
- `bitLengthSlice(bytes: &[]u8): usize`
- `bfldMaskU8(width: u3): u8` | `bfldMaskU16(width: u4): u16` | `bfldMaskU32(width: u5): u32`
- `bfldMaskU64(width: u6): u64` | `bfldFullMaskU64(width: u8): u64`

## Integer Bit Operations (Half-Open `[start, end)`)
- `u8` (index `u3`): `getBitU8(val, bit): bool` | `setBitU8(val, bit, on): u8` | `getBitsU8(val, start, end): u8` | `setBitsU8(val, start, end, bits): u8`
- `u16` (index `u4`): `getBitU16` | `setBitU16` | `getBitsU16` | `setBitsU16`
- `u32` (index `u5`): `getBitU32` | `setBitU32` | `getBitsU32` | `setBitsU32`
- `u64` (index `u6`): `getBitU64` | `setBitU64` | `getBitsU64` | `setBitsU64`
- `u64` full-range (index `u8`, allows `end == 64`): `getBitsRangeU64(val: u64, start: u8, end: u8): u64` | `setBitsRangeU64(val: u64, start: u8, end: u8, bits: u64): u64`
- `usize` (index `u8`): `getBitUsize` | `setBitUsize` | `getBitsUsize` | `setBitsUsize`

## Slice Bit Operations
- `getBitSlice(bytes: &[]u8, bit_index: usize): bool`
- `setBitSlice(bytes: []u8, bit_index: usize, on: bool)`
- `toggleBitSlice(bytes: []u8, bit_index: usize)`
- `getBitsSlice(bytes: &[]u8, start: usize, end: usize): u64`
- `setBitsSlice(bytes: []u8, start: usize, end: usize, value: u64)`
- `countOnesSlice(bytes: &[]u8): usize`

## `BitArray` Class
```cp
class BitArray {
    data: []u8,
    fn fromSlice(buf: []u8): BitArray;
    fn bitLength(): usize;
    fn getBit(bit_index: usize): bool;
    fn setBit(bit_index: usize, on: bool);
    fn toggleBit(bit_index: usize);
    fn getBits(start: usize, end: usize): u64;
    fn setBits(start: usize, end: usize, value: u64);
    fn countOnes(): usize;
    fn clearAll();
    fn setAll();
}
```

## Gotchas
1. Integer `setBit*`/`setBits*` are **pure** and return the updated integer; slice & `BitArray` setters mutate in place.
2. When accessing bits up to bit 63 inclusive (`end == 64`) on `u64`, use `getBitsRangeU64`/`setBitsRangeU64` (takes `u8`), because `u6` maxes out at 63.
3. All bit ranges `[start, end)` are half-open.
