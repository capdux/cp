# ws — AI Agent Quick Ref

Zero-heap RFC 6455 WebSocket binary frame header encoder/decoder, XOR masking, and close status codec for CP (compatible with npm ws & Rust tungstenite) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FrameHeader,
    isControlOpcode,
    isValidOpcode,
    applyMask,
    decodeHeader,
    encodeHeader,
    encodeFrame,
    encodeClosePayload,
} from "ws";
```

## Core Types & Constants

```cp
class FrameHeader { fin: bool, rsv: u8, opcode: u8, masked: bool, /* +3 fields */ }
```

## API Signatures

```cp
fn isControlOpcode(opcode: u8): bool;
fn isValidOpcode(opcode: u8): bool;
fn applyMask(payload: []u8, mask_key: [4]u8);
fn decodeHeader(data: &[]u8, out: &mut FrameHeader): i32;
fn encodeHeader(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, out: []u8): usize;
fn encodeFrame(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload: &[]u8, out: []u8): usize;
fn encodeClosePayload(code: u16, reason: string, out: []u8): usize;
fn isValidCloseCode(code: u16): bool;
```

## Minimal Usage

```cp
import { encodeClosePayload } from "ws";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeClosePayload(0, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ws"`; install via `cap add ws`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
