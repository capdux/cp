# @cap/quinn-varint

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:quinn-proto` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn)) / `crates.io:quinn` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn))（原库名 `quinn-proto` → CP 包名 `@cap/quinn-varint`）

Pure CP, zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (parity with Rust `quinn-proto`).

## Features

- **RFC 9000 Section 16 VarInt**:
  - `MAX_VARINT` (`2^62 - 1 = 4611686018427387903`)
  - `encodedLen(val: u64): usize` (`1` / `2` / `4` / `8` bytes, `0` on overflow)
  - `encodeVarint(val: u64, out: []u8): usize`
  - `decodeVarint(buf: []u8): QuicVarint`
- **RFC 9000 Section 17 Packet Header**:
  - `parseLongHeader(pkt: []u8): QuicLongHeader` (Form bit, Fixed bit, Packet Type Initial/0-RTT/Handshake/Retry, 32-bit Version, DCID/SCID lengths, Short Header discrimination)

## License

MIT
