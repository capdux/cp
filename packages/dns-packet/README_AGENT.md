# dns-packet — AI Agent Quick Ref

Zero-heap RFC 1035 DNS binary packet encoder/decoder with 12-byte header, domain label encoding, 0xC0 compression pointer decoding, and query builder. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DnsHeader,
    NameDecodeResult,
    putBeU16,
    getBeU16,
    encodeHeader,
    decodeHeader,
    encodeName,
    decodeName,
} from "dns-packet";
```

## Core Types & Constants

```cp
class DnsHeader { id: u16, flags: u16, qdcount: u16, ancount: u16, /* +2 fields */ }
class NameDecodeResult { name_len: usize, next_offset: usize, ok: bool }
```

## API Signatures

```cp
fn putBeU16(dst: []u8, offset: usize, v: u16);
fn getBeU16(src: &[]u8, offset: usize): u16;
fn encodeHeader(h: DnsHeader, dst: []u8): usize;
fn decodeHeader(src: &[]u8): DnsHeader;
fn encodeName(domain: &[]u8, dst: []u8): i64;
fn decodeName(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult;
fn encodeQuery(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64;
```

## Minimal Usage

```cp
import { getBeU16 } from "dns-packet";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getBeU16(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dns-packet"`; install via `cap add dns-packet`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
