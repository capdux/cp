[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# zigzag

Zero-allocation ZigZag signed↔unsigned integer bijection (`i8`/`i16`/`i32`/`i64` scalars and slices) plus Protobuf/Avro/Parquet-compatible ZigZag-Varint streaming codec in pure CP.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `zigzag` |
| **CP Package** | `zigzag` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: zigzag)
use zigzag::ZigZag;
let enc: u64 = ZigZag::encode(-1_i64); // 1
let dec: i64 = ZigZag::decode(enc);    // -1
```

```typescript
// CP (same package name: "zigzag")
import { encodeI64, decodeU64 } from "zigzag";
let enc = encodeI64(-1); // 1
let dec = decodeU64(enc); // -1
```

## Installation

```bash
cap add zigzag
```

## Quick Start & Practical Examples

### 1. Scalar Bijection (`i8`, `i16`, `i32`, `i64`)

Maps small-magnitude signed integers (`0, -1, 1, -2, 2, ...`) to small unsigned integers (`0, 1, 2, 3, 4, ...`) so varint/LEB128 encoders compress negative numbers efficiently.

```typescript
import {
    encodeI8, decodeU8,
    encodeI16, decodeU16,
    encodeI32, decodeU32,
    encodeI64, decodeU64,
} from "zigzag";

fn scalarCodec(): void {
    let e8 = encodeI8(-1);       // 1
    let d8 = decodeU8(e8);       // -1 (alias: decodeI8)

    let e16 = encodeI16(-2);     // 3
    let d16 = decodeU16(e16);    // -2 (alias: decodeI16)

    let e32 = encodeI32(-100);   // 199
    let d32 = decodeU32(e32);    // -100 (alias: decodeI32)

    let e64 = encodeI64(-42);    // 83
    let d64 = decodeU64(e64);    // -42 (alias: decodeI64)
}
```

### 2. Batch Slice Encode & Decode

```typescript
import { encodeI32Slice, decodeU32Slice, encodeI64Slice, decodeU64Slice } from "zigzag";

fn sliceCodec(): void {
    let src: [4]i32 = [0, -1, 1, -1000];
    let mut enc: [4]u32 = undefined;
    let mut dec: [4]i32 = undefined;

    let n_enc = encodeI32Slice(src[0..4], enc[0..4]); // 4
    let n_dec = decodeU32Slice(enc[0..4], dec[0..4]); // 4
}
```

### 3. ZigZag-Varint Streaming & Slice Codec (Protobuf `sint64` / Avro)

```typescript
import {
    varintLen,
    encodeZigzagVarint,
    decodeZigzagVarint,
    encodeZigzagVarintSlice,
    decodeZigzagVarintSlice,
} from "zigzag";

fn varintCodec(): void {
    // Single value
    let needed = varintLen(-150); // 2 bytes
    let mut buf: [16]u8 = undefined;
    let written = encodeZigzagVarint(-150, buf[0..16]);
    let res = decodeZigzagVarint(buf[0..written], 0);
    if (res.valid) {
        // res.value == -150, res.consumed == 2
    }

    // Batch pack / unpack of i64 sequences
    let vals: [4]i64 = [0, -1, 150, -100000];
    let mut packed: [64]u8 = undefined;
    let total_bytes = encodeZigzagVarintSlice(vals[0..4], packed[0..64]);

    let mut unpacked: [4]i64 = undefined;
    let count = decodeZigzagVarintSlice(packed[0..total_bytes], unpacked[0..4]); // 4
}
```

## API Reference

### Scalar Bijection
- `encodeI8(n: i8): u8`, `decodeU8(u: u8): i8` / `decodeI8(u: u8): i8`
- `encodeI16(n: i16): u16`, `decodeU16(u: u16): i16` / `decodeI16(u: u16): i16`
- `encodeI32(n: i32): u32`, `decodeU32(u: u32): i32` / `decodeI32(u: u32): i32`
- `encodeI64(n: i64): u64`, `decodeU64(u: u64): i64` / `decodeI64(u: u64): i64`

### Batch Slice Bijection
- `encodeI8Slice` / `decodeU8Slice`, `encodeI16Slice` / `decodeU16Slice`
- `encodeI32Slice` / `decodeU32Slice`, `encodeI64Slice` / `decodeU64Slice`

### ZigZag-Varint Codec
- `VarintResult { value: i64, consumed: usize, valid: bool }`
- `varintLen(n: i64): usize`
- `encodeZigzagVarint(n: i64, out: []u8): usize`
- `decodeZigzagVarint(data: &[]u8, start: usize): VarintResult`
- `encodeZigzagVarintSlice(src: &[]i64, dst: []u8): usize`
- `decodeZigzagVarintSlice(src: &[]u8, dst: []i64): usize`
