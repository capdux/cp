# bytemuck — AI Agent Quick Ref

Zero-heap safe POD byte casting, LE/BE scalar I/O, unaligned reads, layout checks, and zero-copy slice views (Rust `bytemuck` parity).

## Import

```cp
import {
    POD_OK, POD_ERR_SIZE_MISMATCH, POD_ERR_ALIGNMENT,
    POD_ERR_OUTPUT_SLICE_TOO_SMALL, POD_ERR_TARGET_ALIGNMENT_GREATER,
    CastSliceResult, checkCastLayout, isAligned, fillZeroed,
    bytesOfU16, bytesOfU32, bytesOfU64, bytesOfF32, bytesOfF64,
    fromBytesU16, fromBytesU32, fromBytesU64, fromBytesF32, fromBytesF64,
    podReadUnalignedU16, podReadUnalignedU32, podReadUnalignedU64,
    podReadUnalignedI32, podReadUnalignedF32, podReadUnalignedF64,
    tryCastBytesToU16Slice, tryCastBytesToU32Slice, tryCastBytesToU64Slice,
    castU16SliceAsBytes, castU32SliceAsBytes, castU64SliceAsBytes,
    tryCastSliceBytesToU32, swapBytesU16, swapBytesU32, swapBytesU64,
} from "bytemuck";
```

## Core API Signatures

```cp
// Error codes: POD_OK=0, SIZE_MISMATCH=1, ALIGNMENT=2, OUTPUT_SLICE_TOO_SMALL=3, TARGET_ALIGNMENT_GREATER=4
class CastSliceResult { count: usize, err_code: u8, fn isOk(): bool }
checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8
isAligned(addr: usize, align_req: usize): bool
fillZeroed(buf: []u8); zeroedU8/U16/U32/U64/I32/I64/F32/F64()

// Scalar LE/BE & POD
writeU16Le/Be(v: u16, out: []u8): bool; readU16Le/Be(buf: []u8): u16?
writeU32Le/Be(v: u32, out: []u8): bool; readU32Le/Be(buf: []u8): u32?
writeU64Le/Be(v: u64, out: []u8): bool; readU64Le/Be(buf: []u8): u64?
writeF32Le(v: f32, out: []u8): bool; readF32Le(buf: &[]u8): f32?
writeF64Le(v: f64, out: []u8): bool; readF64Le(buf: &[]u8): f64?
bytesOfU16/U32/U64/F32/F64(v): [N]u8
fromBytesU16/U32/U64/F32/F64(buf: &[]u8): T?          // exact len == sizeof(T)
podReadUnalignedU16/U32/U64/I32/F32/F64(buf: &[]u8): T? // len >= sizeof(T)

// Slice casts & zero-copy views
castU16SliceAsBytes(&[]u16): &[]u8; castU32SliceAsBytes(&[]u32): &[]u8; castU64SliceAsBytes(&[]u64): &[]u8
tryCastSliceBytesToU32(src: &[]u8): (&[]u32)?
tryCastBytesToU16Slice/U32Slice/U64Slice(src: &[]u8, dst: []u*): CastSliceResult
castU16SliceToBytesLe(&[]u16, []u8): usize?; castBytesToU16SliceLe(&[]u8, []u16): usize?
castU32SliceToBytesLe([]u32, []u8): usize?;  castBytesToU32SliceLe([]u8, []u32): usize?
castU64SliceToBytesLe(&[]u64, []u8): usize?; castBytesToU64SliceLe(&[]u8, []u64): usize?
swapBytesU16(v: u16): u16; swapBytesU32(v: u32): u32; swapBytesU64(v: u64): u64
```

## Minimal Example

```cp
import { bytesOfU32, fromBytesU32, castU32SliceAsBytes, tryCastBytesToU32Slice } from "bytemuck";

fn demo(): bool {
    let b = bytesOfU32(0x12345678);
    let ok1 = (fromBytesU32(&b) ?? 0) == 0x12345678;
    let words: [2]u32 = [1, 2];
    let view = castU32SliceAsBytes(&words);
    let out: [2]u32 = undefined;
    return ok1 && tryCastBytesToU32Slice(view, out[0..2]).isOk();
}
```

## Gotchas & Borrow Rules

- **`fromBytes*` vs `podReadUnaligned*`**: `fromBytes*` requires `buf.len == sizeof(T)`; `podReadUnaligned*` accepts any `buf.len >= sizeof(T)`.
- **Slice param signatures**: `readU16/U32/U64Le/Be` and `castU32SliceToBytesLe`/`castBytesToU32SliceLe` take `[]u8`/`[]u32`, while `readF32Le`/`readF64Le`, `fromBytes*`, `podReadUnaligned*`, `tryCastBytesTo*`, and `cast*SliceAsBytes` take `&[]u8`/`&[]u*`.
- **Alignment**: `tryCastSliceBytesToU32` returns `null` if `src.ptr` is not 4-byte aligned or `src.len % 4 != 0`.
