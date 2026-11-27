[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# zerocopy

Zero-heap, zero-copy prefix/suffix byte slice parsers, stateful byte cursors, explicit-endian integer wrappers (`U16LE`/`U16BE`/`U32LE`/`U32BE`/`U64LE`/`U64BE`), and layout-validated slice views for CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/zerocopy](https://crates.io/crates/zerocopy) |
| **Upstream Package** | `zerocopy` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/google/zerocopy |
| **License** | MIT (upstream: BSD-2-Clause OR Apache-2.0 OR MIT) |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust
use zerocopy::{FromBytes, IntoBytes, byteorder::network_endian::U32};
```

```cp
// CP — same package name, zero-heap native performance
import {
    U32BE, ByteCursor, ByteWriterCursor,
    readU32BeFromPrefix, sliceAsBytesU32, refFromBytesSliceU32,
} from "zerocopy";
```

## Installation

```bash
cap add zerocopy
```

## Quick Start & Practical Examples

### 1. Prefix & Suffix Zero-Copy Splitting (`readFromPrefix` / `readFromSuffix` / `writeToPrefix`)

```cp
import {
    readFromPrefix, readFromSuffix,
    readU16BeFromPrefix, readU32BeFromPrefix, readU64LeFromSuffix,
    writeToPrefix, writeToSuffix,
} from "zerocopy";

fn parsePacket(pkt: &[]u8) {
    // Parse a 2-byte big-endian header followed by a 4-byte length
    if (readU16BeFromPrefix(pkt)) |hdr| {
        let msg_type = hdr.value;
        if (readU32BeFromPrefix(hdr.rest)) |len_part| {
            let payload_len = len_part.value as usize;
            if (readFromPrefix(len_part.rest, payload_len)) |split| {
                let payload: &[]u8 = split.head;
                let trailer: &[]u8 = split.tail;
                if (readU64LeFromSuffix(trailer)) |crc| {
                    // crc.value is trailing checksum; crc.rest is preceding bytes
                }
            }
        }
    }

    // Write directly into a mutable destination slice and get the remaining slice
    mut out: [16]u8 = [0; 16];
    let magic: [4]u8 = [0x43, 0x50, 0x00, 0x01];
    if (writeToPrefix(&mut out, &magic)) |rest| {
        let _ = writeToSuffix(rest, &magic);
    }
}
```

### 2. Stateful `ByteCursor` & `ByteWriterCursor`

```cp
import { ByteCursor, ByteWriterCursor } from "zerocopy";

fn demoCursors() {
    mut buf: [32]u8 = [0; 32];
    mut w = ByteWriterCursor.new(buf.len);
    let _ = w.putU8(&mut buf, 1);
    let _ = w.putU16Be(&mut buf, 8080);
    let _ = w.putU32Le(&mut buf, 0x12345678);
    let _ = w.putF64Le(&mut buf, 2.71828);
    let payload: [3]u8 = [65, 66, 67];
    let _ = w.putBytes(&mut buf, &payload);

    let total_written = w.written();
    mut r = ByteCursor.new(total_written);
    if (r.peekU8(&buf)) |first| {
        // Non-advancing peek == 1
    }
    let ver = r.nextU8(&mut buf);
    let port = r.nextU16Be(&mut buf);
    let id = r.nextU32Le(&mut buf);
    let ratio = r.nextF64Le(&buf);
    if (r.nextBytes(&buf, 3)) |tag| {
        // Zero-copy &[]u8 slice pointing into buf
    }
}
```

### 3. Explicit-Endian Value Types (`U16LE`..`U64BE`) & Slice Views

```cp
import {
    U16LE, U16BE, U32LE, U32BE, U64LE, U64BE,
    refFromBytesU32, refFromBytesSliceU32,
    sliceAsBytesU16, sliceAsBytesU32, sliceAsBytesU64,
    isAlignedSlice,
} from "zerocopy";

fn demoEndianTypes() {
    mut net_port = U16BE.new(443);
    let wire_bytes: [2]u8 = net_port.toBytes();
    if (U16BE.fromBytes(&wire_bytes)) |parsed| {
        let port = parsed.get(); // 443
    }
    net_port.set(8443);

    // Zero-copy slice conversions
    let nums: [2]u32 = [10, 20];
    let raw: &[]u8 = sliceAsBytesU32(&nums);
    if (refFromBytesSliceU32(raw)) |view| {
        // view.len == 2 (returns null on misalignment or bad length)
    }
    let ok = isAlignedSlice(raw.len, 4, 0x1000, 4);
}
```

## Performance & Safety Highlights

- **Zero Heap & Zero Copy**: `readFromPrefix`, `readFromSuffix`, `ByteCursor.nextBytes`, and `sliceAsBytesU*` return borrowed sub-slices directly without copying or allocating.
- **Bounds-Safe Cursors**: Every `ByteCursor` / `ByteWriterCursor` operation checks `remaining()` before advancing and returns `null` or `false` on overflow/truncation.
- **Explicit Byte Order**: `U16LE`/`U16BE`/`U32LE`/`U32BE`/`U64LE`/`U64BE` store fixed `[N]u8` arrays so network/wire layouts never depend on host alignment or endianness.
- **Layout Validation**: `isAlignedSlice` and `refFromBytesSliceU32` delegate to `bytemuck` layout checks to guarantee alignment and size safety.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
