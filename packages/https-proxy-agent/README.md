[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# https-proxy-agent

Zero-heap HTTP/HTTPS proxy URL parser and RFC 9110 CONNECT tunnel request builder (covers http-proxy-agent)

| Metadata | Value |
| --- | --- |
| Package | `https-proxy-agent` |
| Version | `7.0.6` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add https-proxy-agent
```

```cp
import {
    ProxyEndpoint,
    hpaParsePort,
    parseProxyUrl,
    hpaWriteBase64,
    buildConnectRequest,
} from "https-proxy-agent";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProxyEndpoint` | `class` | Fields: `scheme: string, host: string, port: u16, username: string, password: string, has_auth: bool, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hpaParsePort` | `(s: string): u16?` | Invokes `hpaParsePort` and returns `u16?`. |
| `parseProxyUrl` | `(url: string): ProxyEndpoint` | Parses input via `parseProxyUrl` and returns `ProxyEndpoint`. |
| `hpaWriteBase64` | `(input: &[]u8, out: []u8, pos: usize): usize` | Invokes `hpaWriteBase64` and returns `usize`. |
| `buildConnectRequest` | `(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize` | Invokes `buildConnectRequest` and returns `usize`. |

## Minimal Example

```cp
import { parseProxyUrl } from "https-proxy-agent";

pub fn main() {
    let result = parseProxyUrl("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
