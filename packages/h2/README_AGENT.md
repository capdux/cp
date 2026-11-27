# h2@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7541 HTTP/2 HPACK static table (61 entries), N-bit prefixed integer codec, and indexed header encoder for CP. | License: MIT
> Import: `import { ... } from "h2";` | Full types: `src/index.d.cp`

## Functions
- `export fn staticTableGet(index: usize): StaticEntry;`
- `export fn staticTableFind(name: string, value: string): usize;`
- `export fn staticTableFindName(name: string): usize;`
- `export fn encodePrefixedInt(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize;`
- `export fn decodePrefixedInt(data: &[]u8, prefix_bits: u8): IntDecodeResult;`
- `export fn encodeIndexedHeader(index: usize, out: []u8): usize;`
