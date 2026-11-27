[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# netstring

Zero-heap D. J. Bernstein Netstring (`<len>:<payload>,`) framing encoder, validator, and streaming decoder in pure CP.

- **Package**: `netstring` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `crates.io/netstring` (`MIT`), designed for SCGI, binary RPC, and zero-copy framing over byte buffers.

## Installation & Import

```bash
cap add netstring
```

```typescript
import {
    NetstringFrame, NetstringDecoder,
    encodedLen, encode, encodeString,
    decode, decodeNext, countFrames, isValidNetstring,
} from "netstring";
```

## Core API

### Frame & Streaming Decoder Classes

| Class | Fields & Methods | Description |
| --- | --- | --- |
| `NetstringFrame` | `status: i32`, `payload_start: usize`, `payload_len: usize`, `consumed: usize`; `isOk(): bool`, `isIncomplete(): bool`, `isInvalid(): bool`, `payload(buf: &[]u8): &[]u8` | Zero-copy decoded frame metadata; `payload(buf)` slices the original buffer directly. |
| `NetstringDecoder` | `offset: usize`; `new(): NetstringDecoder`, `next(buf: &[]u8): NetstringFrame`, `position(): usize`, `reset()` | Stateful cursor for iterating consecutive netstring frames in a buffer. |

### Encoding, Decoding & Validation Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodedLen` | `(payload_len: usize): usize` | Exact wire byte length of `<len>:<payload>,`. |
| `encode` | `(payload: &[]u8, dst: []u8): i64` | Encodes raw bytes into `dst`; returns bytes written or `-1` if `dst` is too small. |
| `encodeString` | `(payload: string, dst: []u8): i64` | Encodes a string into `dst`; returns bytes written or `-1` if `dst` is too small. |
| `decode` | `(buf: &[]u8): NetstringFrame` | Decodes the first frame at offset `0`. |
| `decodeNext` | `(buf: &[]u8, offset: usize): NetstringFrame` | Decodes the next frame starting at `offset`. |
| `countFrames` | `(buf: &[]u8): i64` | Counts complete valid frames in `buf`, or returns `-1` if malformed. |
| `isValidNetstring` | `(buf: &[]u8): bool` | Returns `true` if `buf` is exactly one valid netstring with no trailing bytes. |

## Examples

### 1. Encode & Zero-Copy Decode

```typescript
import { encodeString, decode } from "netstring";

fn main() {
    let wire: [64]u8 = undefined;
    let n = encodeString("hello world!", wire[0..]); // writes "12:hello world!,"
    if (n > 0) {
        let frame = decode(wire[0..n]);
        if (frame.isOk()) {
            let body = frame.payload(wire[0..n]); // zero-copy slice "hello world!"
        }
    }
}
```

### 2. Streaming Multiple Frames with `NetstringDecoder`

```typescript
import { NetstringDecoder, countFrames } from "netstring";

fn parseStream(buf: &[]u8): usize {
    let dec = NetstringDecoder.new();
    let seen: usize = 0;
    while (dec.position() < buf.len) {
        let f = dec.next(buf);
        if (!f.isOk()) { break; }
        let payload = f.payload(buf);
        seen += 1;
    }
    return seen;
}
```

### 3. Pre-Sizing & Validation

```typescript
import { encodedLen, isValidNetstring } from "netstring";

fn main() {
    let needed = encodedLen(12); // 16 bytes for "12:<12 bytes>,"
    let valid = isValidNetstring("5:hello,"); // true
}
```

## Differences from Upstream & Highlights

- **Zero-Copy Payload Views**: `NetstringFrame` stores offsets (`payload_start`, `payload_len`) instead of allocating a `Vec<u8>`; `frame.payload(buf)` returns a read-only borrow into the input buffer.
- **Strict RFC / DJB Compliance**: Rejects leading zeros in length prefixes (e.g. `05:hello,`), missing `:` or trailing `,`, and distinguishes `isIncomplete()` (need more bytes) from `isInvalid()` (protocol error).
- **No Exceptions**: Returns negative `i64` (`-1`) on buffer overflow during encoding and structured `NetstringFrame` status flags on decode.
