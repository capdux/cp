# quinn-proto@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (quinn-proto parity) | License: MIT
> Import: `import { ... } from "quinn-proto";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodedLen(val: u64): usize;`
- `export fn encodeVarint(val: u64, out: []u8): usize;`
- `export fn decodeVarint(buf: []u8): QuicVarint;`
- `export fn parseLongHeader(pkt: []u8): QuicLongHeader;`
