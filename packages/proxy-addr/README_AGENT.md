# proxy-addr — AI Agent Quick Ref

Pure CP zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (npm proxy-addr parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    IpSpan,
    PaIpv4,
    PaIpv6Info,
    paTrimBounds,
    paStripV4Mapped,
    paParseIpv4,
    paParseIpv6Info,
    isLoopback,
} from "proxy-addr";
```

## Core Types & Constants

```cp
class IpSpan { start: usize, len: usize }
class PaIpv4 { ok: bool, o0: u8, o1: u8, o2: u8, /* +1 fields */ }
class PaIpv6Info { ok: bool, is_loopback: bool, first_word: u16 }
```

## API Signatures

```cp
fn paTrimBounds(s: string): IpSpan;
fn paStripV4Mapped(s: string): string;
fn paParseIpv4(s: string): PaIpv4;
fn paParseIpv6Info(s: string): PaIpv6Info;
fn isLoopback(ip: string): bool;
fn isLinkLocal(ip: string): bool;
fn isUniqueLocal(ip: string): bool;
fn isTrustedBuiltin(ip: string, preset: u8): bool;
fn parseXffAddresses(remote_addr: string, xff_header: string, out: []IpSpan): usize;
fn paCopyToOut(src: string, out: []u8): usize;
fn resolveClientIp(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { isLoopback } from "proxy-addr";

pub fn main() {
    let result = isLoopback("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"proxy-addr"`; install via `cap add proxy-addr`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
