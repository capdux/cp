# proxy-protocol-js — AI Agent Quick Ref

Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary) parser and encoder for L4 client IP preservation. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ProxyV1Header,
    ProxyV2Header,
    isV1Prefix,
    isV2Signature,
    parsePortStrict,
    parseV1,
    appendSlice,
    appendU16,
} from "proxy-protocol-js";
```

## Core Types & Constants

```cp
class ProxyV1Header { proto: u8, src_ip: string, dst_ip: string, src_port: u16, /* +2 fields */ }
class ProxyV2Header { is_proxy: bool, family: u8, transport: u8, payload_len: u16, /* +5 fields */ }
```

## API Signatures

```cp
fn isV1Prefix(data: &[]u8): bool;
fn isV2Signature(data: &[]u8): bool;
fn parsePortStrict(s: string): i32;
fn parseV1(data: string, out: &mut ProxyV1Header): i32;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn appendU16(out: []u8, pos: usize, v: u16): i64;
fn encodeV1(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize;
fn parseV2(data: &[]u8, out: &mut ProxyV2Header): i32;
fn encodeV2Ipv4(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize;
```

## Minimal Usage

```cp
import { parsePortStrict } from "proxy-protocol-js";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"proxy-protocol-js"`; install via `cap add proxy-protocol-js`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
