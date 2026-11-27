# ws@1.0.0 (Agent Quickref)

> Zero-heap RFC 6455 WebSocket binary frame header encoder/decoder, XOR masking, and close status codec for CP (compatible with npm ws & Rust tungstenite) | License: MIT
> Import: `import { ... } from "ws";` | Full types: `src/index.d.cp`

## Functions
- `export fn isControlOpcode(opcode: u8): bool;`
- `export fn isValidOpcode(opcode: u8): bool;`
- `export fn applyMask(payload: []u8, mask_key: [4]u8);`
- `export fn decodeHeader(data: &[]u8, out: &mut FrameHeader): i32;`
- `export fn encodeHeader(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, out: []u8): usize;`
- `export fn encodeFrame(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload: &[]u8, out: []u8): usize;`
- `export fn encodeClosePayload(code: u16, reason: string, out: []u8): usize;`
- `export fn isValidCloseCode(code: u16): bool;`
