[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# netstring

Zero-heap D. J. Bernstein Netstring (`<len>:<payload>,`) framing encoder, validator, and streaming decoder in pure CP — ideal for SCGI, IPC, and length-prefixed binary protocols.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `netstring` |
| **CP Package** | `netstring` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: netstring)
let frame = format!("{}:{},", payload.len(), payload);
```

```typescript
// CP (same package name: "netstring")
import { encodeString, decode } from "netstring";
let mut buf: [64]u8 = undefined;
let n = encodeString("hello world!", buf[0..64]); // "12:hello world!,"
```

## Installation

```bash
cap add netstring
```

## Quick Start & Practical Examples

### 1. Encoding Bytes & Strings into Caller Buffers

```typescript
import { encodedLen, encode, encodeString } from "netstring";

fn encodeExamples(): void {
    // Compute exact wire length before allocating a buffer
    let need = encodedLen(12); // "12:" (3) + 12 + "," (1) = 16

    let mut buf: [64]u8 = undefined;
    let n1 = encodeString("hello world!", buf[0..64]); // 16 ("12:hello world!,")

    let raw: [4]u8 = [0x01, 0x02, 0x00, 0xFF];
    let n2 = encode(raw[0..4], buf[0..64]); // 7 ("4:\x01\x02\x00\xFF,")
}
```

### 2. Zero-Copy Decoding, Validation & Frame Counting

```typescript
import { decode, decodeNext, countFrames, isValidNetstring } from "netstring";

fn decodeExamples(wire: &[]u8): void {
    if (!isValidNetstring(wire)) {
        return;
    }

    let frame = decode(wire);
    if (frame.isOk()) {
        // Zero-copy slice into `wire`
        let body = frame.payload(wire);
        // Decode the next frame starting at `frame.consumed`
        let next_frame = decodeNext(wire, frame.consumed);
    }

    // Count complete frames in a concatenated buffer (-1 if invalid)
    let total = countFrames(wire);
}
```

### 3. Stateful Streaming Decoder (`NetstringDecoder`)

```typescript
import { NetstringDecoder } from "netstring";

fn parseStream(wire: &[]u8): void {
    let mut dec = NetstringDecoder.new();
    while (dec.position() < wire.len) {
        let frame = dec.next(wire);
        if (frame.isOk()) {
            let body = frame.payload(wire);
            // process `body` (zero-copy view)
        } else if (frame.isIncomplete()) {
            // Wait for more bytes from socket/pipe
            break;
        } else {
            // frame.isInvalid() — malformed length prefix or missing trailing ','
            break;
        }
    }
    dec.reset();
}
```

## API Reference

### Constants
- `NS_OK: i32` (`0`), `NS_INCOMPLETE: i32` (`1`), `NS_INVALID: i32` (`-1`)

### Types
- `NetstringFrame { status: i32, payload_start: usize, payload_len: usize, consumed: usize }`:
  - `isOk(): bool`, `isIncomplete(): bool`, `isInvalid(): bool`, `payload(buf: &[]u8): &[]u8`
- `NetstringDecoder { offset: usize }`:
  - `NetstringDecoder.new(): NetstringDecoder`, `next(buf: &[]u8): NetstringFrame`, `position(): usize`, `reset()`

### Functions
- `encodedLen(payload_len: usize): usize`
- `encode(payload: &[]u8, dst: []u8): i64`, `encodeString(payload: string, dst: []u8): i64` (returns `-1` if `dst` is too small)
- `decode(buf: &[]u8): NetstringFrame`, `decodeNext(buf: &[]u8, offset: usize): NetstringFrame`
- `countFrames(buf: &[]u8): i64`, `isValidNetstring(buf: &[]u8): bool`
