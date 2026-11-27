# dns-packet@1.0.0 (Agent Quickref)

> Zero-heap RFC 1035 DNS binary packet encoder/decoder with 12-byte header, domain label encoding, 0xC0 compression pointer decoding, and query builder. | License: MIT
> Import: `import { ... } from "dns-packet";` | Full types: `src/index.d.cp`

## Functions
- `export fn putBeU16(dst: []u8, offset: usize, v: u16);`
- `export fn getBeU16(src: &[]u8, offset: usize): u16;`
- `export fn encodeHeader(h: DnsHeader, dst: []u8): usize;`
- `export fn decodeHeader(src: &[]u8): DnsHeader;`
- `export fn encodeName(domain: &[]u8, dst: []u8): i64;`
- `export fn decodeName(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult;`
- `export fn encodeQuery(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64;`
