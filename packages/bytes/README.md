[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `bytes`

Unified zero-heap byte size parser/formatter (IEC `KiB`/`MiB`/`GiB` & SI `KB`/`MB`/`GB`) and zero-copy binary buffer cursors (`BytesReader` / `BytesMut`) for CP — a clean-room superset of **npm `bytes`** and **crates.io `bytes`**.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registries** | `npm:bytes` ([visionmedia/bytes.js](https://github.com/visionmedia/bytes.js), v3 parity) & `crates.io:bytes` ([tokio-rs/bytes](https://github.com/tokio-rs/bytes), v1 parity) |
| **CP Package** | `bytes@1.0.0` (`MIT`) |
| **Dependencies** | None (stdlib `cap:bin`, `cap:fmt`, `cap:ascii` only) |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm bytes)
import bytes from "bytes";
const n = bytes.parse("1.5MB"); // 1572864
```

```rust
// Rust (crates.io bytes)
use bytes::{Buf, BufMut, BytesMut};
let mut buf = BytesMut::new();
buf.put_u32(0x12345678);
```

```cp
// CP — identical package name, both APIs unified
import { parse, formatIec, BytesMut, BytesReader } from "bytes";

const sz = parse("1.5MB"); // 1572864
let bm = BytesMut.new();
_ = bm.putU32Be(0x12345678);
```

## Architecture & Zero-Heap Design

- **Zero dynamic allocations**: `BytesReader` borrows `&[]u8` with a zero-copy cursor; `BytesMut` uses an inline 512-byte stack buffer (`BYTES_MUT_CAP = 512`).
- **Endian-safe binary I/O**: Built directly on `cap:bin` (`readU16BE/LE`, `readU32BE/LE`, `readU64BE/LE`, `writeU*`) for native LLVM codegen in `ReleaseFast`.
- **Dual unit systems**: Supports both IEC binary (`KIB = 1024`, `MIB`, `GIB`, `TIB`, `PIB`) and SI decimal (`KB = 1000`, `MB`, `GB`, `TB`, `PB`) parsing and formatting.

## Examples

### 1. Human-Readable Byte Size Parsing & Formatting

```cp
import { parse, parseBytes, formatBytes, formatIec, formatSi } from "bytes";

fn demoByteSizes() {
    const a = parse("1.5MB");              // 1572864 (binary default)
    const b = parseBytes("1.5MB", false);  // 1500000 (SI decimal mode)
    const bad = parse("invalid");          // -1 on parse failure

    let buf: [64]u8 = undefined;
    const n1 = formatIec(1572864, buf[0..]);           // "1.5 MiB"
    const n2 = formatSi(1500000, buf[0..]);            // "1.5 MB"
    const n3 = formatBytes(1536, true, 2, buf[0..]);   // "1.50 KiB"
    _ = a; _ = b; _ = bad; _ = n1; _ = n2; _ = n3;
}
```

### 2. Zero-Copy Binary Framing with `BytesMut` & `BytesReader`

```cp
import { BytesMut, BytesReader } from "bytes";

fn demoBinaryCodec() {
    // Encode packet header + payload on stack (no heap)
    let w = BytesMut.new();
    if (!w.putU16Be(0x0102)) { return; }
    if (!w.putU32Le(42)) { return; }
    if (!w.putSlice("hello")) { return; }

    let wire: [64]u8 = undefined;
    const total = w.copyAll(wire[0..]);

    // Decode with bounds-checked BytesReader
    let r = BytesReader.new(wire[0..total]);
    const magic = r.getU16Be();
    const seq = r.getU32Le();
    if (!magic.ok || !seq.ok) { return; }

    let payload: [5]u8 = undefined;
    if (!r.copyToSlice(payload[0..])) { return; }
    // magic.val == 0x0102, seq.val == 42, payload == "hello"
}
```

### 3. Edge Cases: `splitTo`, Capacity Checks & Bounds Safety

```cp
import { BytesMut, BytesReader } from "bytes";

fn demoEdgeCases() {
    let w = BytesMut.new();
    _ = w.putSlice("header:body");

    // Split prefix out into caller buffer (returns -1 if at > used or out too small)
    let head: [16]u8 = undefined;
    const split_len = w.splitTo(7, head[0..]); // 7 ("header:"), remaining in w is "body"

    // Truncated read safely returns ok == false without advancing past end
    let r = BytesReader.new(head[0..2]);
    const u64_res = r.getU64Be(); // u64_res.ok == false
    _ = split_len; _ = u64_res;
}
```

## API Overview

- **Size Parsing / Formatting**: `parse`, `parseBytes`, `formatBytes`, `formatIec`, `formatSi`, `KIB`..`PIB`, `KB`..`PB`
- **Binary Reader**: `BytesReader` (`new`, `remaining`, `hasRemaining`, `position`, `advance`, `getU8`, `getU16Be`/`Le`, `getU32Be`/`Le`, `getU64Be`/`Le`, `copyToSlice`)
- **Binary Writer**: `BytesMut` (`new`, `len`, `capacity`, `remainingMut`, `isEmpty`, `clear`, `truncate`, `byteAt`, `putU8`, `putU16Be`/`Le`, `putU32Be`/`Le`, `putU64Be`/`Le`, `putSlice`, `splitTo`, `copyAll`)

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
