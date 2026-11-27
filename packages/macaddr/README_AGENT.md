# macaddr@1.0.0 (Agent Quickref)

> Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 (MacAddr6) and EUI-64 (MacAddr8) hardware address parser, formatter, and RFC 4291 converter (crates.io macaddr equivalent) | License: MIT
> Import: `import { ... } from "macaddr";` | Full types: `src/index.d.cp`

## Functions
- `export fn macHexVal(ch: u8): u8?;`
- `export fn macParseHexByte(hi_ch: u8, lo_ch: u8): u8?;`
- `export fn macHexDigit(n: u8): u8;`
- `export fn macWriteHexByte(b: u8, out: []u8, pos: usize): usize;`
