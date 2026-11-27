[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# bytemuck

Zero-heap, safe POD (Plain Old Data) byte reinterpretation, little/big-endian integer & float byte casting, unaligned reads, and alignment/size layout guard checks for CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/bytemuck](https://crates.io/crates/bytemuck) |
| **Upstream Package** | `bytemuck` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/Lokathor/bytemuck |
| **License** | MIT (upstream: Zlib OR Apache-2.0 OR MIT) |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust
use bytemuck::{bytes_of, from_bytes, pod_read_unaligned, cast_slice, try_cast_slice};
```

```cp
// CP — same package name, zero-heap native performance
import {
    bytesOfU32, fromBytesU32, podReadUnalignedU32,
    castU32SliceAsBytes, tryCastSliceBytesToU32,
} from "bytemuck";
```

## Installation

```bash
cap add bytemuck
```

## Quick Start & Practical Examples

### 1. Value-to-Bytes & Bytes-to-Value (`bytesOf*` / `fromBytes*` / `podReadUnaligned*`)

```cp
import {
    bytesOfU16, bytesOfU32, bytesOfU64, bytesOfF32, bytesOfF64,
    fromBytesU32, fromBytesF64, podReadUnalignedU32, podReadUnalignedI32,
} from "bytemuck";

fn demoValueCasting() {
    let raw32: [4]u8 = bytesOfU32(0x12345678);
    if (fromBytesU32(&raw32)) |v| {
        // v == 0x12345678
    }

    let raw_f64: [8]u8 = bytesOfF64(3.1415926535);
    if (fromBytesF64(&raw_f64)) |f| {
        // f == 3.1415926535
    }

    // Safe unaligned read from any byte offset (returns null if buf.len < size)
    let unaligned_slice: &[]u8 = &raw_f64[1..5];
    if (podReadUnalignedU32(unaligned_slice)) |u| {
        // Safely decoded without UB
    }
    if (podReadUnalignedI32(unaligned_slice)) |s| {
        // Signed 32-bit unaligned read
    }
}
```

### 2. Explicit Endianness & Byte Swapping

```cp
import {
    writeU16Le, readU16Le, writeU16Be, readU16Be,
    writeU32Le, readU32Le, writeU32Be, readU32Be,
    writeU64Le, readU64Le, writeU64Be, readU64Be,
    writeF32Le, readF32Le, writeF64Le, readF64Le,
    swapBytesU16, swapBytesU32, swapBytesU64,
} from "bytemuck";

fn demoEndian() {
    mut buf: [8]u8 = [0; 8];
    let ok = writeU32Be(0xDEADBEEF, &mut buf[0..4]);
    if (ok) {
        if (readU32Be(&mut buf[0..4])) |v| {
            let swapped = swapBytesU32(v); // 0xEFBEADDE
        }
    }

    let _ = writeF32Le(1.5, &mut buf[0..4]);
    if (readF32Le(&buf[0..4])) |f| {
        // f == 1.5
    }
}
```

### 3. Zero-Copy & Checked Slice Casting (`cast*Slice*` / `tryCast*`)

```cp
import {
    castU16SliceAsBytes, castU32SliceAsBytes, castU64SliceAsBytes,
    tryCastSliceBytesToU32, tryCastBytesToU32Slice,
    castU32SliceToBytesLe, castBytesToU32SliceLe,
    checkCastLayout, isAligned, CastSliceResult,
} from "bytemuck";

fn demoSliceCasting() {
    let words: [2]u32 = [0x01020304, 0x05060708];
    // Zero-copy view of &[]u32 as &[]u8
    let bytes: &[]u8 = castU32SliceAsBytes(&words);

    // Zero-copy checked cast back to &[]u32 (returns null if misaligned or length % 4 != 0)
    if (tryCastSliceBytesToU32(bytes)) |back| {
        // back.len == 2
    }

    // Checked copy-based cast with explicit status code
    mut dst: [2]u32 = [0; 2];
    let res: CastSliceResult = tryCastBytesToU32Slice(bytes, &mut dst);
    if (res.isOk()) {
        // res.count == 2
    }

    // Manual layout & alignment guard
    let aligned = isAligned(0x1000, 4); // true
    let code = checkCastLayout(bytes.len, 4, 0x1000, 4); // 0 when valid
}
```

### 4. Zeroed POD Helpers

```cp
import {
    fillZeroed, zeroedU8, zeroedU16, zeroedU32, zeroedU64,
    zeroedI32, zeroedI64, zeroedF32, zeroedF64,
} from "bytemuck";

fn demoZeroed() {
    mut scratch: [16]u8 = [0xFF; 16];
    fillZeroed(&mut scratch); // All bytes set to 0
    let z = zeroedU64();      // 0
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: All functions operate strictly on stack values, fixed-size arrays (`[2]u8`, `[4]u8`, `[8]u8`), and borrowed slices (`&[]u8`, `[]u8`).
- **Alignment & Size Guards**: `tryCastSliceBytesToU32`, `tryCastBytesTo*Slice`, `isAligned`, and `checkCastLayout` reject misaligned pointers and non-multiple byte lengths without undefined behavior.
- **Unaligned Read Safety**: `podReadUnaligned*` and `read*Le` / `read*Be` safely decode arbitrary byte offsets and return `null` when the input slice is too short.
- **Borrow-Checked Zero-Copy Views**: `castU16SliceAsBytes`, `castU32SliceAsBytes`, and `castU64SliceAsBytes` expose read-only `&[]u8` views with zero copying.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
