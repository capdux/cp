[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fast-socks5

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 username/password authentication frame encoder and decoder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fast-socks5` ([https://github.com/dizda/fast-socks5](https://github.com/dizda/fast-socks5)) / `npm:socks` ([https://github.com/JoshGlazebrook/socks](https://github.com/JoshGlazebrook/socks))（原库名 `fast-socks5` → CP 包名 `fast-socks5`）

| Metadata | Value |
| --- | --- |
| Package | `fast-socks5` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fast-socks5
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Socks5Request` | `class` | Fields: `cmd: u8, atyp: u8, ipv4: [4]u8, ipv6: [16]u8, domain: string, port: u16, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodeGreeting` | `(methods: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeGreeting` returning `usize`. |
| `selectMethod` | `(greeting: &[]u8, supported: &[]u8, out_method: &mut u8): i32` | Invokes `selectMethod` and returns `i32`. |
| `encodeUserPassAuth` | `(user: string, pass: string, out: []u8): usize` | Serializes/formats data via `encodeUserPassAuth` returning `usize`. |
| `encodeConnectDomain` | `(host: string, port: u16, out: []u8): usize` | Serializes/formats data via `encodeConnectDomain` returning `usize`. |
| `encodeConnectIpv4` | `(ip: [4]u8, port: u16, out: []u8): usize` | Serializes/formats data via `encodeConnectIpv4` returning `usize`. |
| `decodeRequest` | `(data: &[]u8, out: &mut Socks5Request): i32` | Parses input via `decodeRequest` and returns `i32`. |
| `encodeReplyIpv4` | `(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize` | Serializes/formats data via `encodeReplyIpv4` returning `usize`. |

## Minimal Example

```cp
import { encodeUserPassAuth } from "fast-socks5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUserPassAuth("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
