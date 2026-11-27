# zerocopy — AI Agent Quick Ref

- **Import**: `import { ... } from "zerocopy";`
- **Origin**: crates.io `zerocopy` (v1.0.0, MIT, zero-heap, depends on `bytemuck`)
- **Full Types**: `src/index.d.cp`

## Core Classes

### `ByteCursor`
- `ByteCursor.new(total_len: usize): ByteCursor`
- `remaining(): usize` | `reset()` | `seek(offset: usize): bool` | `skip(n: usize): bool`
- `peekU8(buf: &[]u8): u8?` | `nextBytes(buf: &[]u8, n: usize): (&[]u8)?`
- `nextU8(buf: []u8): u8?` | `nextU16Le/Be(buf: []u8): u16?` | `nextU32Le/Be(buf: []u8): u32?`
- `nextU64Le/Be(buf: []u8): u64?` | `nextI32Le(buf: []u8): i32?`
- `nextF32Le(buf: &[]u8): f32?` | `nextF64Le(buf: &[]u8): f64?`

### `ByteWriterCursor`
- `ByteWriterCursor.new(cap_len: usize): ByteWriterCursor`
- `written(): usize` | `remaining(): usize` | `reset()`
- `putU8/putU16Le/Be/putU32Le/Be/putU64Le/Be/putI32Le/putF32Le/putF64Le(out: []u8, v): bool`
- `putBytes(out: []u8, src: &[]u8): bool`

### Explicit-Endian Wrappers (`U16LE`, `U16BE`, `U32LE`, `U32BE`, `U64LE`, `U64BE`)
- `T.new(v: uN): T` | `T.fromBytes(buf: &[]u8): T?`
- `get(): uN` | `set(v: uN)` | `toBytes(): [N]u8`

### Split Result Classes
- `SplitBytes { head: &[]u8, tail: &[]u8 }`
- `PrefixU16 { value: u16, rest: &[]u8 }` (also `PrefixU32`, `PrefixU64`)

## Free Functions
- `readFromPrefix(buf: &[]u8, n: usize): SplitBytes?` | `readFromSuffix(buf: &[]u8, n: usize): SplitBytes?`
- `readU16LeFromPrefix` / `readU16BeFromPrefix` / `readU32LeFromPrefix` / `readU32BeFromPrefix` / `readU64LeFromPrefix` / `readU64BeFromPrefix`
- `readU16LeFromSuffix` / `readU32LeFromSuffix` / `readU64LeFromSuffix`
- `writeToPrefix(dst: []u8, src: &[]u8): ([]u8)?` | `writeToSuffix(dst: []u8, src: &[]u8): ([]u8)?`
- `refFromBytesU16(buf: &[]u8): u16?` | `refFromBytesU32` | `refFromBytesU64`
- `refFromBytesSliceU32(buf: &[]u8): (&[]u32)?`
- `sliceAsBytesU16(src: &[]u16): &[]u8` | `sliceAsBytesU32` | `sliceAsBytesU64`
- `isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool`

## Gotchas
1. `ByteCursor.nextU8/nextU16*/nextU32*/nextU64*/nextI32Le` take `buf: []u8`, whereas `peekU8`, `nextF32Le`, `nextF64Le`, `nextBytes` take `buf: &[]u8`.
2. `writeToPrefix` / `writeToSuffix` return the **remaining** unwritten slice `([]u8)?` (or `null` if `src.len > dst.len`).
3. `ByteCursor` does not own the buffer — always pass the same backing buffer across calls.
