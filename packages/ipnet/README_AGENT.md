# ipnet — AI Agent Quick Ref

Zero-heap pure-CP IPv4/CIDR subnet calculation engine (parse, format, masks, containment, supernet, host count — crates.io ipnet equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Ipv4Net,
    ipnParseIpv4Range,
    ipnParsePrefixRange,
    ipnWriteU8Dec,
    ipnNetmask,
    parseIpv4,
    formatIpv4,
} from "ipnet";
```

## Core Types & Constants

```cp
class Ipv4Net { addr: u32, prefix_len: u8 } // methods: new(...) +11
```

## API Signatures

```cp
fn ipnParseIpv4Range(s: string, start: usize, end: usize): u32?;
fn ipnParsePrefixRange(s: string, start: usize, end: usize): u8?;
fn ipnWriteU8Dec(v: u8, out: []u8, pos: usize): usize;
fn ipnNetmask(prefix_len: u8): u32;
fn parseIpv4(s: string): u32?;
fn formatIpv4(ip: u32, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseIpv4 } from "ipnet";

pub fn main() {
    let result = parseIpv4("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ipnet"`; install via `cap add ipnet`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
