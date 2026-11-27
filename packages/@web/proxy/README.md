[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/proxy

Official zero-heap HTTP reverse proxy plugin for cap:web (prefix stripping, traversal guard, HTTP/1.1 formatter, pluggable transport)

| Metadata | Value |
| --- | --- |
| Package | `@web/proxy` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/proxy
```

```cp
import {
    ProxyConfig,
    ProxyRequestPlan,
    pxAsciiLower,
    pxWriteRaw,
    pxWriteByte,
    pxWriteU64,
    pxHasControlChars,
    pxHasTraversal,
} from "@web/proxy";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProxyConfig` | `class` | Fields: `prefix: string, upstream_host: string, upstream_port: u16, strip_prefix: bool, add_forwarded_headers: bool` |
| `ProxyRequestPlan` | `class` | Fields: `target_path: [256]u8, target_path_len: usize, host_header: [128]u8, host_header_len: usize, forwarded_for: string, forwarded_proto: string` · Methods: `targetPathSlice, hostHeaderSlice` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pxAsciiLower` | `(c: u8): u8` | Invokes `pxAsciiLower` and returns `u8`. |
| `pxWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `pxWriteRaw` and returns `bool`. |
| `pxWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `pxWriteByte` and returns `bool`. |
| `pxWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `pxWriteU64` and returns `bool`. |
| `pxHasControlChars` | `(s: string): bool` | Invokes `pxHasControlChars` and returns `bool`. |
| `pxHasTraversal` | `(path: string): bool` | Invokes `pxHasTraversal` and returns `bool`. |
| `pxTrimTrailingSlash` | `(s: string): string` | Invokes `pxTrimTrailingSlash` and returns `string`. |
| `pxMatchesPrefix` | `(req_path: string, raw_prefix: string): bool` | Invokes `pxMatchesPrefix` and returns `bool`. |
| `pxDefaultTransport` | `(cfg: &ProxyConfig, ctx: &mut Context, target_path: string): Response` | Invokes `pxDefaultTransport` and returns `Response`. |
| `buildProxyTargetPath` | `(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): usize?` | Invokes `buildProxyTargetPath` and returns `usize?`. |
| `buildProxyRequestPlan` | `(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ProxyRequestPlan?` | Invokes `buildProxyRequestPlan` and returns `ProxyRequestPlan?`. |
| `formatUpstreamHttp1Request` | `(method: AppMethod, target_path: string, body: string, cfg: ProxyConfig, client_ip: string, out_buf: []u8): usize?` | Serializes/formats data via `formatUpstreamHttp1Request` returning `usize?`. |
| `proxyHook` | `(ctx: &mut Context): bool` | Invokes `proxyHook` and returns `bool`. |
| `createProxyPlugin` | `(cfg: ProxyConfig, transport: ProxyTransportFn?): Plugin` | Constructs/initializes an instance via `createProxyPlugin`. |
| `registerProxy` | `(app: &mut App, cfg: ProxyConfig): &mut App` | Registers plugin/middleware into the target application. |
| `registerProxyWithTransport` | `(app: &mut App, cfg: ProxyConfig, transport: ProxyTransportFn): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { formatUpstreamHttp1Request } from "@web/proxy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUpstreamHttp1Request(/* method: AppMethod */, "example", "example", /* cfg: ProxyConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
