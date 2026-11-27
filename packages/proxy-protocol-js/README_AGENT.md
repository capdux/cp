# proxy-protocol-js@1.0.0 (Agent Quickref)

> Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary) parser and encoder for L4 client IP preservation. | License: MIT
> Import: `import { ... } from "proxy-protocol-js";` | Full types: `src/index.d.cp`

## Functions
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn isV1Prefix(data: &[]u8): bool;`
- `export fn isV2Signature(data: &[]u8): bool;`
- `export fn parsePortStrict(s: string): i32;`
- `export fn parseV1(data: string, out: &mut ProxyV1Header): i32;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn appendU16(out: []u8, pos: usize, v: u16): i64;`
- `export fn encodeV1(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize;`
- `export fn parseV2(data: &[]u8, out: &mut ProxyV2Header): i32;`
- `export fn encodeV2Ipv4(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize;`
