# ipnet@1.0.0 (Agent Quickref)

> Zero-heap pure-CP IPv4/CIDR subnet calculation engine (parse, format, masks, containment, supernet, host count — crates.io ipnet equivalent) | License: MIT
> Import: `import { ... } from "ipnet";` | Full types: `src/index.d.cp`

## Functions
- `export fn ipnParseIpv4Range(s: string, start: usize, end: usize): u32?;`
- `export fn ipnParsePrefixRange(s: string, start: usize, end: usize): u8?;`
- `export fn ipnWriteU8Dec(v: u8, out: []u8, pos: usize): usize;`
- `export fn ipnNetmask(prefix_len: u8): u32;`
- `export fn parseIpv4(s: string): u32?;`
- `export fn formatIpv4(ip: u32, out: []u8): usize;`
