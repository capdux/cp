# bytemuck — AI Agent Quick Ref

- **Import**: `import { ... } from "bytemuck";`
- **Origin**: crates.io `bytemuck` (v1.0.0, MIT, zero-heap)
- **Full Types**: `src/index.d.cp`

## Core Types

```cp
class CastSliceResult {
    count: usize,
    err_code: u8,
    fn isOk(): bool;
}
```

## Function Signatures

### Layout & Zeroed
- `checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8`
- `isAligned(addr: usize, align_req: usize): bool`
- `fillZeroed(buf: []u8)`
- `zeroedU8(): u8` / `zeroedU16(): u16` / `zeroedU32(): u32` / `zeroedU64(): u64`
- `zeroedI32(): i32` / `zeroedI64(): i64` / `zeroedF32(): f32` / `zeroedF64(): f64`

### Endian Read / Write & Byte Swap
- `writeU16Le(v: u16, out: []u8): bool` | `readU16Le(buf: []u8): u16?` (also `Be`, `U32`, `U64`)
- `writeF32Le(v: f32, out: []u8): bool` | `readF32Le(buf: &[]u8): f32?`
- `writeF64Le(v: f64, out: []u8): bool` | `readF64Le(buf: &[]u8): f64?`
- `swapBytesU16(v: u16): u16` | `swapBytesU32(v: u32): u32` | `swapBytesU64(v: u64): u64`

### Value <-> Bytes & Unaligned Read
- `bytesOfU16(v: u16): [2]u8` | `bytesOfU32(v: u32): [4]u8` | `bytesOfU64(v: u64): [8]u8`
- `bytesOfF32(v: f32): [4]u8` | `bytesOfF64(v: f64): [8]u8`
- `fromBytesU16(buf: &[]u8): u16?` | `fromBytesU32(buf: &[]u8): u32?` | `fromBytesU64(buf: &[]u8): u64?`
- `fromBytesF32(buf: &[]u8): f32?` | `fromBytesF64(buf: &[]u8): f64?`
- `podReadUnalignedU16(buf: &[]u8): u16?` | `...U32` | `...U64` | `...I32` | `...F32` | `...F64`

### Slice Casting (Copy & Zero-Copy)
- `castU16SliceToBytesLe(src: &[]u16, dst: []u8): usize?` | `castBytesToU16SliceLe(src: &[]u8, dst: []u16): usize?`
- `castU32SliceToBytesLe(src: []u32, dst: []u8): usize?` | `castBytesToU32SliceLe(src: []u8, dst: []u32): usize?`
- `castU64SliceToBytesLe(src: &[]u64, dst: []u8): usize?` | `castBytesToU64SliceLe(src: &[]u8, dst: []u64): usize?`
- `tryCastBytesToU16Slice(src: &[]u8, dst: []u16): CastSliceResult` (also `U32`, `U64`)
- `castU16SliceAsBytes(src: &[]u16): &[]u8` | `castU32SliceAsBytes(src: &[]u32): &[]u8` | `castU64SliceAsBytes(src: &[]u64): &[]u8`
- `tryCastSliceBytesToU32(src: &[]u8): (&[]u32)?`

## Gotchas
1. `readU16Le/Be`, `readU32Le/Be`, `readU64Le/Be`, `castU32SliceToBytesLe`, and `castBytesToU32SliceLe` take mutable slices (`[]u8` / `[]u32`), whereas `fromBytes*`, `podReadUnaligned*`, and `cast*SliceAsBytes` take `&[]u8` / `&[]uN`.
2. `tryCastSliceBytesToU32` returns `null` if `src.len % 4 != 0` or pointer is not 4-byte aligned; use `tryCastBytesToU32Slice` or `podReadUnalignedU32` for unaligned buffers.
3. All `write*` functions return `false` (no panic) if `out.len` is smaller than the type size.
