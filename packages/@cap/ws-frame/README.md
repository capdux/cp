# @cap/ws-frame

Zero-heap RFC 6455 WebSocket binary frame header encoder/decoder, 4-byte XOR masking, and close status codec for CP.
Compatible with npm `ws` and Rust `tungstenite`.

## API

- Opcodes: `OP_CONTINUATION`, `OP_TEXT`, `OP_BINARY`, `OP_CLOSE`, `OP_PING`, `OP_PONG`
- Close codes: `CLOSE_NORMAL`, `CLOSE_GOING_AWAY`, `CLOSE_PROTOCOL_ERROR`, `CLOSE_POLICY_VIOLATION`, `CLOSE_INTERNAL_ERROR`
- `FrameHeader`, `decodeHeader`, `encodeHeader`, `encodeFrame`, `applyMask`, `encodeClosePayload`, `isValidCloseCode`

## License

MIT
