[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zerocopy

Zero-heap zero-copy prefix/suffix byte slice readers, writers, endian-aware byte wrappers, and alignment-checked slice casts for CP, modeled on Google's Rust [`zerocopy`](https://github.com/google/zerocopy) crate (MIT, CP v1.0.0, built on `bytemuck`).

## Installation & Import

```bash
cap add zerocopy
```

```cp
import {
    ByteCursor,
    ByteWriterCursor,
    U16LE,
    U32BE,
    readU32BeFromPrefix,
    readFromPrefix,
    sliceAsBytesU32,
} from "zerocopy";
```

## Core API Reference

### Cursor Readers & Writers

| Type | Methods | Description |
| :--- | :--- | :--- |
| `ByteCursor` | `new(total_len)`, `remaining()`, `reset()`, `seek(offset): bool`, `skip(n): bool` | Stateful offset tracker for sequential binary decoding over caller-owned buffers. |
| `ByteCursor` (read) | `peekU8(&[]u8): u8?`, `nextU8([]u8): u8?`, `nextU16Le/Be([]u8): u16?`, `nextU32Le/Be([]u8): u32?`, `nextU64Le/Be([]u8): u64?`, `nextI32Le([]u8): i32?`, `nextF32Le(&[]u8): f32?`, `nextF64Le(&[]u8): f64?`, `nextBytes(&[]u8, n): (&[]u8)?` | Bounds-checked sequential reads; returns `null` if remaining bytes are insufficient. |
| `ByteWriterCursor` | `new(cap_len)`, `written()`, `remaining()`, `reset()`, `putU8`, `putU16Le/Be`, `putU32Le/Be`, `putU64Le/Be`, `putI32Le`, `putF32Le`, `putF64Le`, `putBytes(out: []u8, src: &[]u8): bool` | Bounds-checked sequential binary writer returning `false` on overflow. |

### Unaligned Endian Wrapper Types

All 6 types (`U16LE`, `U16BE`, `U32LE`, `U32BE`, `U64LE`, `U64BE`) store an internal fixed byte array `[N]u8` (alignment 1) and expose:

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new(v)` | `(T): Self` | Encodes integer `v` in the specified byte order. |
| `fromBytes(buf)` | `(&[]u8): Self?` | Reads the first `N` bytes (unaligned-safe); returns `null` if `buf.len < N`. |
| `get()` / `set(v)` | `(): T` / `(T)` | Decodes / updates the stored integer value. |
| `toBytes()` | `(): [N]u8` | Returns the raw byte array. |

### Prefix / Suffix Splitting & Layout-Checked Casts

| Function | Signature | Description |
| :--- | :--- | :--- |
| `readFromPrefix` / `readFromSuffix` | `(buf: &[]u8, n: usize): SplitBytes?` | Splits `buf` into `{ head: &[]u8, tail: &[]u8 }` without copying. |
| `readU16LeFromPrefix` / `readU16BeFromPrefix` | `(buf: &[]u8): PrefixU16?` | Reads prefix `u16` and returns `{ value, rest: &[]u8 }`. |
| `readU32LeFromPrefix` / `readU32BeFromPrefix` | `(buf: &[]u8): PrefixU32?` | Reads prefix `u32` and returns `{ value, rest: &[]u8 }`. |
| `readU64LeFromPrefix` / `readU64BeFromPrefix` | `(buf: &[]u8): PrefixU64?` | Reads prefix `u64` and returns `{ value, rest: &[]u8 }`. |
| `readU16LeFromSuffix` / `readU32LeFromSuffix` / `readU64LeFromSuffix` | `(buf: &[]u8): PrefixU*?` | Reads little-endian integer from the tail and returns the remaining prefix in `rest`. |
| `writeToPrefix` / `writeToSuffix` | `(dst: []u8, src: &[]u8): ([]u8)?` | Copies `src` to the start/end of `dst`, returning the remaining writable slice. |
| `isAlignedSlice` | `(byte_len, elem_size, addr, align_req): bool` | Validates size divisibility and address alignment via `bytemuck.checkCastLayout`. |
| `refFromBytesU16` / `U32` / `U64` | `(buf: &[]u8): u16? \| u32? \| u64?` | Alignment- and size-checked direct pointer load; returns `null` if misaligned or wrong size. |
| `refFromBytesSliceU32` | `(buf: &[]u8): (&[]u32)?` | Zero-copy cast from `&[]u8` to `&[]u32` when aligned to 4 bytes and length is a multiple of 4. |
| `sliceAsBytesU16` / `U32` / `U64` | `(src: &[]u*): &[]u8` | Zero-copy reinterpretation of integer slices as read-only byte slices. |

## Usage Examples

### 1. Zero-Copy Packet Header Decoding (Prefix Chaining)

```cp
import { readU16BeFromPrefix, readU32BeFromPrefix, readFromPrefix } from "zerocopy";

fn parsePacket(pkt: &[]u8): bool {
    let ver = readU16BeFromPrefix(pkt) ?? return false;
    let payload_len = readU32BeFromPrefix(ver.rest) ?? return false;
    let split = readFromPrefix(payload_len.rest, payload_len.value as usize) ?? return false;
    // split.head is a zero-copy &[]u8 view of the payload; split.tail is remaining bytes
    return ver.value == 1 && split.head.len == (payload_len.value as usize);
}
```

### 2. Encode & Decode with `ByteWriterCursor` and `ByteCursor`

```cp
import { ByteWriterCursor, ByteCursor } from "zerocopy";

fn cursorRoundTrip() {
    let buf: [32]u8 = undefined;
    let mut w = ByteWriterCursor.new(32);
    w.putU32Le(buf[0..32], 0xDEADBEEF);
    w.putF64Le(buf[0..32], 3.14159);

    let mut r = ByteCursor.new(w.written());
    let magic = r.nextU32Le(buf[0..32]);
    let pi = r.nextF64Le(&buf[0..32]);
}
```

### 3. Unaligned Endian Wrappers (`U32BE`)

```cp
import { U32BE } from "zerocopy";

fn endianExample() {
    let net_val = U32BE.new(8080);
    let raw = net_val.toBytes(); // [0x00, 0x00, 0x1F, 0x90]
    let parsed = U32BE.fromBytes(&raw);
    // parsed.?.get() == 8080
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Explicit Typed Functions Over Derive Macros**: Replaces Rust's `#[derive(FromBytes, IntoBytes)]` traits with explicit, compile-time-safe functions and endian wrappers (`U16LE`..`U64BE`) tailored to CP's type system.
- **No Undefined Behavior**: Alignment-sensitive operations (`refFromBytesU32`, `refFromBytesSliceU32`) verify pointer alignment and byte-length divisibility at runtime and return `null` instead of causing misaligned pointer UB.
- **Zero-Allocation Slicing**: `readFromPrefix`, `readFromSuffix`, `nextBytes`, and `sliceAsBytes*` return borrowed `&[]u8` views with zero copying.
