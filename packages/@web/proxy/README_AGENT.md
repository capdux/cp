# @web/proxy — AI Agent Quick Ref

Official zero-heap HTTP reverse proxy plugin for cap:web (prefix stripping, traversal guard, HTTP/1.1 formatter, pluggable transport) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class ProxyConfig { prefix: string, upstream_host: string, upstream_port: u16, strip_prefix: bool, /* +1 fields */ }
class ProxyRequestPlan { target_path: [256]u8, target_path_len: usize, host_header: [128]u8, host_header_len: usize, /* +2 fields */ } // methods: targetPathSlice(...) +1
```

## API Signatures

```cp
fn pxAsciiLower(c: u8): u8;
fn pxWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn pxWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn pxWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn pxHasControlChars(s: string): bool;
fn pxHasTraversal(path: string): bool;
fn pxTrimTrailingSlash(s: string): string;
fn pxMatchesPrefix(req_path: string, raw_prefix: string): bool;
fn pxDefaultTransport(cfg: &ProxyConfig, ctx: &mut Context, target_path: string): Response;
fn buildProxyTargetPath(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): usize?;
fn buildProxyRequestPlan(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ProxyRequestPlan?;
fn formatUpstreamHttp1Request(method: AppMethod, target_path: string, body: string, cfg: ProxyConfig, client_ip: string, out_buf: []u8): usize?;
fn proxyHook(ctx: &mut Context): bool;
fn createProxyPlugin(cfg: ProxyConfig, transport: ProxyTransportFn?): Plugin;
fn registerProxy(app: &mut App, cfg: ProxyConfig): &mut App;
fn registerProxyWithTransport(app: &mut App, cfg: ProxyConfig, transport: ProxyTransportFn): &mut App;
```

## Minimal Usage

```cp
import { formatUpstreamHttp1Request } from "@web/proxy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUpstreamHttp1Request(/* method: AppMethod */, "example", "example", /* cfg: ProxyConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/proxy"`; install via `cap add @web/proxy`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
