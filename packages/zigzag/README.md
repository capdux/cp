[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zigzag

Zero-allocation 8/16/32/64-bit ZigZag signed-to-unsigned bijection and ZigZag-Varint streaming codec (Protobuf / Avro / Parquet / CBOR) in pure CP.

- **Package**: `zigzag` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/zigzag` (`MIT`), extended with batch slice codecs and LEB128 ZigZag-Varint streaming primitives.

## Installation & Import

```bash
cap add zigzag
```

```typescript
import {
    VarintResult,
    encodeI32, decodeI32,
    encodeI64, decodeI64,
    encodeZigzagVarint, decodeZigzagVarint,
    varintLen,
} from "zigzag";
```

## Core API

### Data Types

| Type | Fields | Description |
| --- | --- | --- |
| `VarintResult` | `value: i64`, `consumed: usize`, `valid: bool` | Decoded ZigZag-Varint value, byte count consumed, and validity flag. |

### Scalar ZigZag Bijection

| Function | Signature | Description |
| --- | --- | --- |
| `encodeI8` | `(n: i8): u8` | Maps signed `i8` to `u8` (`0 -> 0, -1 -> 1, 1 -> 2, ...`). |
| `decodeU8` / `decodeI8` | `(u: u8): i8` | Inverse bijection from `u8` back to `i8`. |
| `encodeI16` | `(n: i16): u16` | Maps signed `i16` to `u16`. |
| `decodeU16` / `decodeI16` | `(u: u16): i16` | Inverse bijection from `u16` back to `i16`. |
| `encodeI32` | `(n: i32): u32` | Maps signed `i32` to `u32`. |
| `decodeI32` / `decodeU32` | `(u: u32): i32` | Inverse bijection from `u32` back to `i32`. |
| `encodeI64` | `(n: i64): u64` | Maps signed `i64` to `u64`. |
| `decodeI64` / `decodeU64` | `(u: u64): i64` | Inverse bijection from `u64` back to `i64`. |

### Batch Slice Codecs

| Function | Signature | Description |
| --- | --- | --- |
| `encodeI8Slice` | `(src: &[]i8, dst: []u8): usize` | Batch-encodes `src` into `dst`, returns element count written (`min(src.len, dst.len)`). |
| `decodeU8Slice` | `(src: &[]u8, dst: []i8): usize` | Batch-decodes `u8` slice into `i8` slice. |
| `encodeI16Slice` | `(src: &[]i16, dst: []u16): usize` | Batch-encodes `i16` slice into `u16` slice. |
| `decodeU16Slice` | `(src: &[]u16, dst: []i16): usize` | Batch-decodes `u16` slice into `i16` slice. |
| `encodeI32Slice` | `(src: &[]i32, dst: []u32): usize` | Batch-encodes `i32` slice into `u32` slice. |
| `decodeU32Slice` | `(src: &[]u32, dst: []i32): usize` | Batch-decodes `u32` slice into `i32` slice. |
| `encodeI64Slice` | `(src: &[]i64, dst: []u64): usize` | Batch-encodes `i64` slice into `u64` slice. |
| `decodeU64Slice` | `(src: &[]u64, dst: []i64): usize` | Batch-decodes `u64` slice into `i64` slice. |

### ZigZag-Varint Streaming Codec

| Function | Signature | Description |
| --- | --- | --- |
| `varintLen` | `(n: i64): usize` | Computes exact encoded LEB128 ZigZag-Varint byte length (`1..=10`). |
| `encodeZigzagVarint` | `(n: i64, out: []u8): usize` | Encodes `n` into `out`; returns bytes written (`0` if buffer too small). |
| `decodeZigzagVarint` | `(data: &[]u8, start: usize): VarintResult` | Decodes one ZigZag-Varint starting at `start`; check `.valid`. |
| `encodeZigzagVarintSlice` | `(src: &[]i64, dst: []u8): usize` | Encodes a slice of `i64` values sequentially into `dst`, returns total bytes written. |
| `decodeZigzagVarintSlice` | `(src: &[]u8, dst: []i64): usize` | Decodes sequential ZigZag-Varints from `src` into `dst`, returns element count decoded. |

## Examples

### 1. Scalar Bijection

```typescript
import { encodeI32, decodeI32, encodeI64, decodeI64 } from "zigzag";

fn main() {
    let u = encodeI32(-1);        // 1
    let s = decodeI32(u);         // -1
    let big = encodeI64(-100000); // 199999
    let orig = decodeI64(big);    // -100000
}
```

### 2. Streaming ZigZag-Varint Encoding & Decoding

```typescript
import { encodeZigzagVarint, decodeZigzagVarint, varintLen } from "zigzag";

fn main() {
    let buf: [16]u8 = undefined;
    let needed = varintLen(-300);
    let written = encodeZigzagVarint(-300, buf[0..]);
    let res = decodeZigzagVarint(buf[0..written], 0);
    if (res.valid) {
        // res.value == -300, res.consumed == needed
    }
}
```

### 3. Batch Slice Round-Trip

```typescript
import { encodeZigzagVarintSlice, decodeZigzagVarintSlice } from "zigzag";

fn main() {
    let nums: [4]i64 = [-1, 0, 1, -1000];
    let wire: [40]u8 = undefined;
    let bytes_written = encodeZigzagVarintSlice(nums[0..], wire[0..]);

    let decoded: [4]i64 = undefined;
    let count = decodeZigzagVarintSlice(wire[0..bytes_written], decoded[0..]);
    // count == 4, decoded == [-1, 0, 1, -1000]
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All batch and Varint APIs write directly into caller-provided slices (`dst: []u8`, `out: []u8`) with no allocator required.
- **Explicit Width Functions**: Replaces Rust `ZigZag` trait dispatch with monomorphic `encodeI8`/`I16`/`I32`/`I64` and `decodeU*`/`decodeI*` aliases for zero-overhead inlining.
- **Safe Varint Bounds**: `decodeZigzagVarint` guards against truncated streams and >10-byte overflow varints via `VarintResult.valid`.
