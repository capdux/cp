# fast-socks5 — AI Agent Quick Ref

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 username/password authentication frame encoder and decoder for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Socks5Request,
    encodeGreeting,
    selectMethod,
    encodeUserPassAuth,
    encodeConnectDomain,
    encodeConnectIpv4,
    decodeRequest,
    encodeReplyIpv4,
} from "fast-socks5";
```

## Core Types & Constants

```cp
class Socks5Request { cmd: u8, atyp: u8, ipv4: [4]u8, ipv6: [16]u8, /* +3 fields */ }
```

## API Signatures

```cp
fn encodeGreeting(methods: &[]u8, out: []u8): usize;
fn selectMethod(greeting: &[]u8, supported: &[]u8, out_method: &mut u8): i32;
fn encodeUserPassAuth(user: string, pass: string, out: []u8): usize;
fn encodeConnectDomain(host: string, port: u16, out: []u8): usize;
fn encodeConnectIpv4(ip: [4]u8, port: u16, out: []u8): usize;
fn decodeRequest(data: &[]u8, out: &mut Socks5Request): i32;
fn encodeReplyIpv4(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize;
```

## Minimal Usage

```cp
import { encodeUserPassAuth } from "fast-socks5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUserPassAuth("example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fast-socks5"`; install via `cap add fast-socks5`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
