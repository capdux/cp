# zigzag (AI Agent Quick Ref)

Zero-alloc 8/16/32/64-bit ZigZag bijection + LEB128 ZigZag-Varint streaming codec.

## Import
```typescript
import {
    VarintResult,
    encodeI8, decodeU8, decodeI8,
    encodeI16, decodeU16, decodeI16,
    encodeI32, decodeI32, decodeU32,
    encodeI64, decodeI64, decodeU64,
    encodeI8Slice, decodeU8Slice, encodeI16Slice, decodeU16Slice,
    encodeI32Slice, decodeU32Slice, encodeI64Slice, decodeU64Slice,
    varintLen, encodeZigzagVarint, decodeZigzagVarint,
    encodeZigzagVarintSlice, decodeZigzagVarintSlice,
} from "zigzag";
```

## API Signatures
- `class VarintResult { value: i64, consumed: usize, valid: bool }`
- Scalar: `encodeI8(i8): u8`, `decodeU8/decodeI8(u8): i8`, `encodeI16(i16): u16`, `decodeU16/decodeI16(u16): i16`, `encodeI32(i32): u32`, `decodeI32/decodeU32(u32): i32`, `encodeI64(i64): u64`, `decodeI64/decodeU64(u64): i64`
- Slice: `encodeI{8,16,32,64}Slice(src: &[]iN, dst: []uN): usize`, `decodeU{8,16,32,64}Slice(src: &[]uN, dst: []iN): usize`
- Varint:
  - `varintLen(n: i64): usize` (`1..=10`)
  - `encodeZigzagVarint(n: i64, out: []u8): usize` (`0` on insufficient buffer)
  - `decodeZigzagVarint(data: &[]u8, start: usize): VarintResult`
  - `encodeZigzagVarintSlice(src: &[]i64, dst: []u8): usize`
  - `decodeZigzagVarintSlice(src: &[]u8, dst: []i64): usize`

## Minimal Example
```typescript
import { encodeZigzagVarint, decodeZigzagVarint } from "zigzag";

fn roundtrip(n: i64): i64 {
    let buf: [10]u8 = undefined;
    let w = encodeZigzagVarint(n, buf[0..]);
    let r = decodeZigzagVarint(buf[0..w], 0);
    if (r.valid) { return r.value; }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- Always reserve at least 10 bytes per `i64` for worst-case Varint encoding (`varintLen(n)` gives exact size).
- `src` slices are read-only borrows (`&[]T`); `dst`/`out` slices are mutable output buffers (`[]T`).
- Always check `res.valid` after `decodeZigzagVarint`; truncated or >10-byte inputs return `valid = false, consumed = 0`.
- `encodeZigzagVarint` returns `0` when `out.len` is too small; advance stream cursor by returned byte count.
- `decodeI32`/`decodeU32` (and 8/16/64 counterparts) are exact aliases taking unsigned input and returning signed output.
