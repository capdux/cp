[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# flate2

Pure CP, zero-heap **RFC 1952 Gzip** and **RFC 1950 Zlib** stored-DEFLATE framing codec with strict **CRC-32**, **ISIZE**, and **Adler-32** integrity verification.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/flate2) |
| **Upstream Package** | `flate2` (v1 parity) |
| **Upstream Repo** | https://github.com/rust-lang/flate2-rs |
| **CP Package** | `flate2` (`1.0.0`) |
| **License** | MIT / Apache-2.0 |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use flate2::write::{GzEncoder, ZlibEncoder};
use flate2::read::{GzDecoder, ZlibDecoder};
```

```typescript
// CP (cap)
import {
    encodeGzipStored,
    decodeGzipStored,
    encodeZlibStored,
    decodeZlibStored,
    parseGzipHeader,
} from "flate2";
```

## Installation

```bash
cap add flate2
```

## Key Features & Performance Highlights

- **Zero Heap Allocation**: Encodes and decodes directly into caller-provided `[]u8` buffers, returning `usize?` (`null` on truncation, overflow, or checksum mismatch).
- **Strict Integrity Validation**:
  - Gzip (`RFC 1952`): Validates magic bytes (`0x1f 0x8b`), method (`8`), optional FEXTRA/FNAME/FCOMMENT/FHCRC header fields, stored DEFLATE `LEN`/`NLEN` complement check, CRC-32, and ISIZE.
  - Zlib (`RFC 1950`): Validates CMF/FLG header (`(CMF*256 + FLG) % 31 == 0`), rejects unsupported preset dictionaries, checks stored DEFLATE blocks, and verifies big-endian Adler-32 trailer.
- **Deterministic Framing**: Computes exact encoded length via `fl2StoredEncodedLen(payload_len)` before writing.

## Quick Start

### Gzip Encode, Header Inspection & Decode

```typescript
import {
    encodeGzipStored,
    decodeGzipStored,
    parseGzipHeader,
} from "flate2";

fn main(): void {
    let payload = "Hello flate2 in CP!";
    let frame_buf: [256]u8 = undefined;

    // Encode RFC 1952 Gzip frame with mtime = 1700000000
    const encoded_len = encodeGzipStored(payload, 1700000000, frame_buf[0..]);
    if (encoded_len == null) {
        return;
    }
    const frame = frame_buf[0..encoded_len.?];

    // Inspect Gzip header metadata
    const hdr = parseGzipHeader(frame);
    if (!hdr.valid) {
        return;
    }

    // Decode with strict CRC-32 + ISIZE verification
    let out_buf: [256]u8 = undefined;
    const decoded_len = decodeGzipStored(frame, out_buf[0..]);
    if (decoded_len == null) {
        return;
    }
}
```

### Zlib Encode & Decode

```typescript
import { encodeZlibStored, decodeZlibStored } from "flate2";

fn roundtripZlib(payload: []u8, tmp: []u8, out: []u8): usize? {
    const zlen = encodeZlibStored(payload, tmp);
    if (zlen == null) {
        return null;
    }
    return decodeZlibStored(tmp[0..zlen.?], out);
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `GZIP_ID1` / `GZIP_ID2` / `GZIP_CM_DEFLATE` | `31` / `139` / `8` | RFC 1952 Gzip header constants |
| `ZLIB_CMF_DEFAULT` / `ZLIB_FLG_NO_COMP` | `120` / `1` | RFC 1950 Zlib header constants |
| `GzipHeader` | `{ method, flags, mtime, os, data_offset, valid }` | Parsed RFC 1952 Gzip header |
| `parseGzipHeader` | `(frame: []u8) -> GzipHeader` | Parse and validate Gzip header and find DEFLATE offset |
| `encodeGzipStored` | `(payload: []u8, mtime: u32, out: []u8) -> usize?` | Encode payload into RFC 1952 Gzip frame |
| `decodeGzipStored` | `(frame: []u8, out: []u8) -> usize?` | Decode Gzip frame with CRC-32 & ISIZE checks |
| `encodeZlibStored` | `(payload: []u8, out: []u8) -> usize?` | Encode payload into RFC 1950 Zlib frame |
| `decodeZlibStored` | `(frame: []u8, out: []u8) -> usize?` | Decode Zlib frame with Adler-32 check |
| `fl2StoredEncodedLen` | `(payload_len: usize) -> usize` | Exact byte length of stored DEFLATE blocks |
| `fl2EncodeStoredBlocks` | `(payload: []u8, out: []u8, start: usize) -> usize` | Low-level RFC 1951 stored-block encoder |
| `fl2DecodeStoredBlocks` | `(stream: []u8, out: []u8) -> usize?` | Low-level RFC 1951 stored-block decoder |

## License

MIT / Apache-2.0
