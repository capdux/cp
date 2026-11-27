# netstring@1.0.0 (Agent Quickref)

> Zero-heap D. J. Bernstein Netstring (len:payload,) framing encoder and streaming decoder in pure CP | License: MIT
> Import: `import { ... } from "netstring";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodedLen(payload_len: usize): usize;`
- `export fn encode(payload: &[]u8, dst: []u8): i64;`
- `export fn decode(buf: &[]u8): NetstringFrame;`
