# @cap/byteorder

Zero-heap big-endian / little-endian binary codecs and bounds-checked forward-only `ByteReader` / `ByteWriter` in pure CP.

Inspired by Rust's `byteorder` (993M+ downloads) and `untrusted` (1.02B+ downloads): eliminates out-of-bounds reads, integer overflow slicing, and partial-parse bugs when parsing network protocols, binary formats, and certificates.

## Install

```bash
cap add @cap/byteorder
```

## Usage

```cp
import { ByteWriter, ByteReader } from "@cap/byteorder";

let buf: [64]u8 = undefined;
let w = ByteWriter.init(buf[0..64]);
* = w.writeU16Be(0xCAFE);
* = w.writeU32Le(0x12345678);
* = w.writeBytes("CP");

let r = ByteReader.init(w.asSlice());
const magic = r.readU16Be() ?? 0;   // 0xCAFE
const id    = r.readU32Le() ?? 0;   // 0x12345678
const tag   = r.readBytes(2) ?? ""; // "CP"
```
