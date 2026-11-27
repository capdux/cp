# zerocopy — AI Agent Quick Ref

Zero-heap prefix/suffix byte cursors, unaligned endian wrappers (`U16LE`..`U64BE`), and alignment-checked zero-copy casts (Rust `zerocopy` parity).

## Import

```cp
import {
    ByteCursor, ByteWriterCursor,
    U16LE, U16BE, U32LE, U32BE, U64LE, U64BE,
    SplitBytes, PrefixU16, PrefixU32, PrefixU64,
    readFromPrefix, readFromSuffix,
    readU16LeFromPrefix, readU32BeFromPrefix, readU64LeFromPrefix,
    writeToPrefix, writeToSuffix,
    refFromBytesSliceU32, sliceAsBytesU32, isAlignedSlice,
} from "zerocopy";
```

## Core API Signatures

```cp
// Stateful Cursors
class ByteCursor { pos: usize, total: usize }
ByteCursor.new(total_len: usize): ByteCursor
c.remaining(): usize; c.reset(); c.seek(offset: usize): bool; c.skip(n: usize): bool
c.peekU8(buf: &[]u8): u8?; c.nextBytes(buf: &[]u8, n: usize): (&[]u8)?
c.nextF32Le(buf: &[]u8): f32?; c.nextF64Le(buf: &[]u8): f64?
c.nextU8(buf: []u8): u8?; c.nextU16Le/Be(buf: []u8): u16?
c.nextU32Le/Be(buf: []u8): u32?; c.nextU64Le/Be(buf: []u8): u64?; c.nextI32Le(buf: []u8): i32?

class ByteWriterCursor { pos: usize, cap: usize }
ByteWriterCursor.new(cap_len: usize): ByteWriterCursor
w.written(): usize; w.remaining(): usize; w.reset()
w.putU8/putU16Le/putU16Be/putU32Le/putU32Be/putU64Le/putU64Be/putI32Le/putF32Le/putF64Le(out: []u8, v): bool
w.putBytes(out: []u8, src: &[]u8): bool

// Endian Wrappers (U16LE, U16BE, U32LE, U32BE, U64LE, U64BE)
U32BE.new(v: u32): U32BE; U32BE.fromBytes(buf: &[]u8): U32BE?
x.get(): u32; x.set(v: u32); x.toBytes(): [4]u8

// Prefix / Suffix & Casts
readFromPrefix(buf: &[]u8, n: usize): SplitBytes? // { head: &[]u8, tail: &[]u8 }
readFromSuffix(buf: &[]u8, n: usize): SplitBytes?
readU16LeFromPrefix/readU16BeFromPrefix/readU16LeFromSuffix(buf: &[]u8): PrefixU16?
readU32LeFromPrefix/readU32BeFromPrefix/readU32LeFromSuffix(buf: &[]u8): PrefixU32?
readU64LeFromPrefix/readU64BeFromPrefix/readU64LeFromSuffix(buf: &[]u8): PrefixU64?
writeToPrefix(dst: []u8, src: &[]u8): ([]u8)?; writeToSuffix(dst: []u8, src: &[]u8): ([]u8)?
refFromBytesU16/U32/U64(buf: &[]u8): u16?|u32?|u64?
refFromBytesSliceU32(buf: &[]u8): (&[]u32)?
sliceAsBytesU16/U32/U64(src: &[]u*): &[]u8
isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool
```

## Minimal Example

```cp
import { readU32BeFromPrefix, readFromPrefix } from "zerocopy";

fn decode(pkt: &[]u8): (&[]u8)? {
    let hdr = readU32BeFromPrefix(pkt) ?? return null;
    let body = readFromPrefix(hdr.rest, hdr.value as usize) ?? return null;
    return body.head;
}
```

## Gotchas & Borrow Rules

- **Slice mutability mismatch on `ByteCursor`**: `nextU8`/`nextU16*`/`nextU32*`/`nextU64*`/`nextI32Le` take `buf: []u8`, whereas `peekU8`, `nextF32Le`, `nextF64Le`, and `nextBytes` take `buf: &[]u8`.
- **Alignment checks**: `refFromBytesU16/U32/U64` and `refFromBytesSliceU32` return `null` on unaligned pointers; use `U*LE`/`U*BE` or `read*FromPrefix` for arbitrary byte offsets.
- **Lifetime**: Slices returned in `SplitBytes`, `PrefixU*`, `nextBytes`, and `sliceAsBytes*` borrow from the input slice with zero copy.
