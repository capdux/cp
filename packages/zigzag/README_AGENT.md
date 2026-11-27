# zigzag — AI Agent Quick Ref

- **Import**: `import { VarintResult, encodeI8, decodeU8, encodeI16, decodeU16, encodeI32, decodeU32, encodeI64, decodeU64, encodeI8Slice, decodeU8Slice, encodeI16Slice, decodeU16Slice, encodeI32Slice, decodeU32Slice, encodeI64Slice, decodeU64Slice, varintLen, encodeZigzagVarint, decodeZigzagVarint, encodeZigzagVarintSlice, decodeZigzagVarintSlice } from "zigzag";`
- **Source**: crates.io `zigzag` (`v1.0.0`, zero-heap)

## Core Signatures
```cp
class VarintResult { value: i64, consumed: usize, valid: bool }

// Scalar bijection (decodeI* are aliases for decodeU*)
encodeI8(n: i8): u8;       decodeU8(u: u8): i8;     decodeI8(u: u8): i8;
encodeI16(n: i16): u16;    decodeU16(u: u16): i16;  decodeI16(u: u16): i16;
encodeI32(n: i32): u32;    decodeU32(u: u32): i32;  decodeI32(u: u32): i32;
encodeI64(n: i64): u64;    decodeU64(u: u64): i64;  decodeI64(u: u64): i64;

// Batch slice bijection (return count processed = min(src.len, dst.len))
encodeI8Slice(src: &[]i8, dst: []u8): usize;
decodeU8Slice(src: &[]u8, dst: []i8): usize;
encodeI16Slice(src: &[]i16, dst: []u16): usize;
decodeU16Slice(src: &[]u16, dst: []i16): usize;
encodeI32Slice(src: &[]i32, dst: []u32): usize;
decodeU32Slice(src: &[]u32, dst: []i32): usize;
encodeI64Slice(src: &[]i64, dst: []u64): usize;
decodeU64Slice(src: &[]u64, dst: []i64): usize;

// ZigZag + LEB128 Varint codec (max 10 bytes per i64)
varintLen(n: i64): usize;
encodeZigzagVarint(n: i64, out: []u8): usize;             // 0 if out too small
decodeZigzagVarint(data: &[]u8, start: usize): VarintResult;
encodeZigzagVarintSlice(src: &[]i64, dst: []u8): usize;   // total bytes written
decodeZigzagVarintSlice(src: &[]u8, dst: []i64): usize;   // count of i64 decoded
```

## Minimal Example
```cp
let u = encodeI64(-42);                 // 83
let mut buf: [16]u8 = undefined;
let n = encodeZigzagVarint(-150, buf[0..16]);
let r = decodeZigzagVarint(buf[0..n], 0); // r.valid == true, r.value == -150
```

## Pitfalls
- A 64-bit ZigZag-Varint takes up to **10 bytes**; size `out` via `varintLen(n)` or `[10]u8`.
- Always check `res.valid` after `decodeZigzagVarint` (returns `valid = false` on truncated or >10-byte malformed input).
- Advance streaming cursor by `offset += res.consumed` when decoding consecutive varints.
- `encodeZigzagVarint` returns `0` if `out.len < varintLen(n)`.
