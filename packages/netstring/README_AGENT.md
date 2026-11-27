# netstring (AI Agent Quick Ref)

Zero-heap D. J. Bernstein Netstring (`<len>:<payload>,`) encoder, validator, and streaming decoder.

## Import
```typescript
import {
    NetstringFrame, NetstringDecoder,
    encodedLen, encode, encodeString,
    decode, decodeNext, countFrames, isValidNetstring,
} from "netstring";
```

## API Signatures
- `class NetstringFrame { status: i32, payload_start: usize, payload_len: usize, consumed: usize }`
  - `isOk(): bool`, `isIncomplete(): bool`, `isInvalid(): bool`, `payload(buf: &[]u8): &[]u8`
- `class NetstringDecoder { offset: usize }`
  - `new(): NetstringDecoder`, `next(buf: &[]u8): NetstringFrame` (`*self`), `position(): usize`, `reset()` (`*self`)
- `encodedLen(payload_len: usize): usize`
- `encode(payload: &[]u8, dst: []u8): i64` (`-1` if `dst` too small)
- `encodeString(payload: string, dst: []u8): i64` (`-1` if `dst` too small)
- `decode(buf: &[]u8): NetstringFrame`
- `decodeNext(buf: &[]u8, offset: usize): NetstringFrame`
- `countFrames(buf: &[]u8): i64` (`-1` on malformed input)
- `isValidNetstring(buf: &[]u8): bool`

## Minimal Example
```typescript
import { encodeString, decode } from "netstring";

fn roundtrip(msg: string): usize {
    let buf: [128]u8 = undefined;
    let n = encodeString(msg, buf[0..]);
    if (n < 0) { return 0; }
    let f = decode(buf[0..n]);
    if (f.isOk()) { return f.payload(buf[0..n]).len; }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- Always pass the SAME `buf` slice to `frame.payload(buf)` that was passed to `decode` / `decodeNext` / `dec.next`; `payload_start` is an offset into that exact slice.
- `encode` and `encodeString` return `i64`: check `n >= 0` before casting/slicing (`-1` means `dst.len < encodedLen(payload.len)`).
- `NetstringDecoder.next` and `reset` mutate `self` (`*self`); bind with `let dec = NetstringDecoder.new();`.
- On `isIncomplete()`, buffer more bytes and retry; `NetstringDecoder` only advances `offset` when `isOk()` is true.
- Leading zeros in length (except `"0:,"`) are rejected as `isInvalid()`.
