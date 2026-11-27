[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bytemuck

Zero-heap safe Plain-Old-Data (POD) byte reinterpretation, LE/BE integer and float byte casting, unaligned POD reads, alignment/size layout validation, and zero-copy slice views for CP, modeled on Rust's [`bytemuck`](https://github.com/Lokathor/bytemuck) crate (MIT, CP v1.0.0).

## Installation & Import

```bash
cap add bytemuck
```

```cp
import {
    POD_OK,
    POD_ERR_SIZE_MISMATCH,
    POD_ERR_ALIGNMENT,
    CastSliceResult,
    bytesOfU32,
    fromBytesU32,
    podReadUnalignedF64,
    tryCastBytesToU32Slice,
    castU32SliceAsBytes,
    tryCastSliceBytesToU32,
    swapBytesU32,
} from "bytemuck";
```

## Core API Reference

### Error Codes & Layout Helpers

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `POD_OK` / `POD_ERR_SIZE_MISMATCH` / `POD_ERR_ALIGNMENT` / `POD_ERR_OUTPUT_SLICE_TOO_SMALL` / `POD_ERR_TARGET_ALIGNMENT_GREATER` | `0` / `1` / `2` / `3` / `4` | Layout and slice-cast error codes returned by `checkCastLayout` and `CastSliceResult.err_code`. |
| `CastSliceResult` | `count: usize`, `err_code: u8`, `isOk(): bool` | Structured result for `tryCastBytesToU*Slice`. |
| `checkCastLayout` | `(byte_len, elem_size, addr, align_req): u8` | Validates length divisibility and pointer alignment. |
| `isAligned` | `(addr: usize, align_req: usize): bool` | Checks whether `addr % align_req == 0`. |
| `fillZeroed` / `zeroedU8..zeroedF64` | `([]u8)` / `(): T` | Zero-fills a byte slice or returns typed zero values (`Zeroable` parity). |

### Scalar Byte Conversion & Unaligned Reads

| Group | Functions | Notes |
| :--- | :--- | :--- |
| `bytesOf*` | `bytesOfU16`, `bytesOfU32`, `bytesOfU64`, `bytesOfF32`, `bytesOfF64` | Converts scalar POD value to fixed `[N]u8` (little-endian). |
| `fromBytes*` | `fromBytesU16`, `fromBytesU32`, `fromBytesU64`, `fromBytesF32`, `fromBytesF64` | Takes `&[]u8`; requires exact byte length `== sizeof(T)`, else `null`. |
| `podReadUnaligned*` | `podReadUnalignedU16`, `U32`, `U64`, `I32`, `F32`, `F64` | Takes `&[]u8`; safe unaligned read when `buf.len >= sizeof(T)`, else `null`. |
| LE / BE Read & Write | `readU16Le/Be`, `writeU16Le/Be`, `readU32Le/Be`, `writeU32Le/Be`, `readU64Le/Be`, `writeU64Le/Be`, `readF32Le`, `writeF32Le`, `readF64Le`, `writeF64Le` | Explicit endian reads (`T?`) and writes (`bool`). |
| Byte Swap | `swapBytesU16`, `swapBytesU32`, `swapBytesU64` | Reverses byte order of unsigned integers. |

### Slice Casting & Zero-Copy Views

| Function | Signature | Description |
| :--- | :--- | :--- |
| `castU16SliceAsBytes` / `U32` / `U64` | `(src: &[]u*): &[]u8` | Zero-copy reinterpretation of integer slice as `&[]u8`. |
| `tryCastSliceBytesToU32` | `(src: &[]u8): (&[]u32)?` | Zero-copy alignment- and size-checked view from `&[]u8` to `&[]u32`. |
| `tryCastBytesToU16Slice` / `U32` / `U64` | `(src: &[]u8, dst: []u*): CastSliceResult` | Safe LE decoding into `dst` reporting exact `POD_*` error code. |
| `castU16SliceToBytesLe` / `castBytesToU16SliceLe` | `(&[]u16, []u8): usize?` / `(&[]u8, []u16): usize?` | Optional-returning bulk LE slice conversion. |
| `castU32SliceToBytesLe` / `castBytesToU32SliceLe` | `([]u32, []u8): usize?` / `([]u8, []u32): usize?` | Bulk `u32` <-> byte slice LE conversion. |
| `castU64SliceToBytesLe` / `castBytesToU64SliceLe` | `(&[]u64, []u8): usize?` / `(&[]u8, []u64): usize?` | Bulk `u64` <-> byte slice LE conversion. |

## Usage Examples

### 1. Scalar `bytesOf*`, `fromBytes*`, and Unaligned Reads

```cp
import { bytesOfF32, fromBytesF32, podReadUnalignedU32 } from "bytemuck";

fn scalarCast() {
    let raw = bytesOfF32(3.5);
    let val = fromBytesF32(&raw); // 3.5

    let pkt: [6]u8 = [0xFF, 0x01, 0x02, 0x03, 0x04, 0xAA];
    let unaligned = podReadUnalignedU32(&pkt[1..6]); // 0x04030201
}
```

### 2. Zero-Copy Slice Reinterpretation & Checked Casts

```cp
import { castU32SliceAsBytes, tryCastBytesToU32Slice, POD_OK } from "bytemuck";

fn sliceCast() {
    let words: [2]u32 = [0x11223344, 0x55667788];
    let bytes = castU32SliceAsBytes(&words); // Zero-copy 8-byte &[]u8 view

    let out: [2]u32 = undefined;
    let res = tryCastBytesToU32Slice(bytes, out[0..2]);
    // res.isOk() == true, res.err_code == POD_OK, res.count == 2
}
```

## Differences from Upstream & Zero-Copy Highlights

- **No Unsafe Transmutes Required by Callers**: Replaces Rust `unsafe impl Pod` / `Zeroable` traits with pre-verified primitive and slice functions backed by `cap:bin` and explicit alignment checks.
- **Unaligned Safety**: `podReadUnaligned*` and `tryCastBytesTo*Slice` work safely on any byte offset without alignment traps.
- **Zero-Copy Views**: `castU16SliceAsBytes`, `castU32SliceAsBytes`, `castU64SliceAsBytes`, and `tryCastSliceBytesToU32` avoid all copying while enforcing strict alignment and length invariants.
