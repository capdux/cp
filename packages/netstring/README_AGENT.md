# netstring — AI Agent Quick Ref

- **Import**: `import { NS_OK, NS_INCOMPLETE, NS_INVALID, NetstringFrame, NetstringDecoder, encodedLen, encode, encodeString, decode, decodeNext, countFrames, isValidNetstring } from "netstring";`
- **Source**: crates.io `netstring` (`v1.0.0`, zero-heap D. J. Bernstein `<len>:<payload>,` framing)

## Constants
```cp
NS_OK: i32 = 0
NS_INCOMPLETE: i32 = 1
NS_INVALID: i32 = -1
```

## Core Signatures
```cp
class NetstringFrame {
    status: i32,
    payload_start: usize,
    payload_len: usize,
    consumed: usize,
    isOk(): bool;
    isIncomplete(): bool;
    isInvalid(): bool;
    payload(buf: &[]u8): &[]u8;
}

class NetstringDecoder {
    offset: usize,
    static new(): NetstringDecoder;
    next(buf: &[]u8): NetstringFrame;
    position(): usize;
    reset();
}

encodedLen(payload_len: usize): usize;
encode(payload: &[]u8, dst: []u8): i64;          // -1 if dst.len < encodedLen(payload.len)
encodeString(payload: string, dst: []u8): i64;   // -1 if dst too small
decode(buf: &[]u8): NetstringFrame;
decodeNext(buf: &[]u8, offset: usize): NetstringFrame;
countFrames(buf: &[]u8): i64;                    // -1 on invalid frame
isValidNetstring(buf: &[]u8): bool;
```

## Pitfalls
- Leading zeros in length prefix (e.g. `01:a,`) are rejected as `NS_INVALID` per DJB netstring spec; `0:,` is valid.
- `encode` / `encodeString` return `i64`: check `>= 0` before slicing `dst[0..(n as usize)]`.
- `NetstringDecoder.next(buf)` advances `offset` only when `frame.isOk()`; bind decoder with `let mut dec = NetstringDecoder.new()`.
- `frame.payload(buf)` returns a zero-copy slice borrowing `buf` — only call it when `frame.isOk()` is true.
