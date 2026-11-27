[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dns-packet

Zero-heap RFC 1035 DNS binary packet encoder/decoder with 12-byte header, domain label encoding, 0xC0 compression pointer decoding, and query builder.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dns-packet` ([https://github.com/mafintosh/dns-packet](https://github.com/mafintosh/dns-packet))

| Metadata | Value |
| --- | --- |
| Package | `dns-packet` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add dns-packet
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DnsHeader` | `class` | Fields: `id: u16, flags: u16, qdcount: u16, ancount: u16, nscount: u16, arcount: u16` |
| `NameDecodeResult` | `class` | Fields: `name_len: usize, next_offset: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `putBeU16` | `(dst: []u8, offset: usize, v: u16)` | Mutates or appends state via `putBeU16`. |
| `getBeU16` | `(src: &[]u8, offset: usize): u16` | Queries/retrieves data via `getBeU16` returning `u16`. |
| `encodeHeader` | `(h: DnsHeader, dst: []u8): usize` | Serializes/formats data via `encodeHeader` returning `usize`. |
| `decodeHeader` | `(src: &[]u8): DnsHeader` | Parses input via `decodeHeader` and returns `DnsHeader`. |
| `encodeName` | `(domain: &[]u8, dst: []u8): i64` | Serializes/formats data via `encodeName` returning `i64`. |
| `decodeName` | `(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult` | Parses input via `decodeName` and returns `NameDecodeResult`. |
| `encodeQuery` | `(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64` | Serializes/formats data via `encodeQuery` returning `i64`. |

## Minimal Example

```cp
import { getBeU16 } from "dns-packet";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getBeU16(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
