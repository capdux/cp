# `base36` — AI Agent Quick Reference

- **Package**: `base36` (`1.0.0`, MIT)
- **Origin**: `npm:base36` + `crates.io:radix_fmt`
- **Import**: `import { encodeBase36, decodeBase36, encodeRadix, decodeRadix, encodeBytes, decodeBytes, isValidBase36 } from "base36";`

## Types

```cp
class ParseU64Result { value: u64, valid: bool }
class DecodeBytesResult { len: usize, valid: bool }
```

## Core Signatures

```cp
// Base36 u64
fn encodeBase36(val: u64, out: []u8): usize
fn decodeBase36(input: string): ParseU64Result
fn encodeU64(val: u64, out: []u8): usize
fn encodeU64Upper(val: u64, out: []u8): usize
fn decodeU64(input: string): ParseU64Result

// Arbitrary radix (2..=36)
fn encodeRadix(val: u64, radix: u8, uppercase: bool, out: []u8): usize
fn decodeRadix(input: string, radix: u8): ParseU64Result
fn encodedLenU64(val: u64, radix: u8): usize

// Validation & byte slices
fn isValidBase36(input: string): bool
fn isValidBase36String(input: string): bool
fn encodeBytes(data: &[]u8, uppercase: bool, out: []u8): usize
fn decodeBytes(input: string, out: []u8): DecodeBytesResult

// Low-level digits
fn digitChar(v: u8, uppercase: bool): u8
fn digitVal(c: u8): i32
```

## Minimal Pattern

```cp
let buf: [32]u8 = undefined;
let n = encodeBase36(123456, buf[0..32]);
let res = decodeBase36(buf[0..n] as string);
if (res.valid) { /* use res.value */ }
```

## Pitfalls

1. **Zero-heap output**: Encoders return written byte count (`usize`); slice `buf[0..n] as string` to obtain the string.
2. **Buffer size**: Reserve `>= 13` bytes for Base36 `u64`, `>= 64` bytes for Base2 `u64`, or check `encodedLenU64(val, radix)`.
3. **Radix bounds**: `radix` must be `2..=36`; out-of-range radix returns `0` (encode) or `valid: false` (decode).
4. **Always check `.valid`**: `decodeBase36` / `decodeRadix` / `decodeBytes` never throw; inspect `.valid` before using `.value` or `.len`.
5. **Immutable borrow**: `encodeBytes` takes `data: &[]u8` (read-only slice reference).
