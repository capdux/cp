# @web/proxy@1.0.0 (Agent Quickref)

> Official zero-heap HTTP reverse proxy plugin for cap:web (prefix stripping, traversal guard, HTTP/1.1 formatter, pluggable transport) | License: MIT
> Import: `import { ... } from "@web/proxy";` | Full types: `src/index.d.cp`

## Functions
- `export fn pxAsciiLower(c: u8): u8;`
- `export fn pxWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn pxWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn pxWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn pxHasControlChars(s: string): bool;`
- `export fn pxHasTraversal(path: string): bool;`
- `export fn pxTrimTrailingSlash(s: string): string;`
- `export fn pxMatchesPrefix(req_path: string, raw_prefix: string): bool;`
- `export fn pxDefaultTransport(cfg: &ProxyConfig, ctx: &mut Context, target_path: string): Response;`
- `export fn buildProxyTargetPath(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): usize?;`
- `export fn buildProxyRequestPlan(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ProxyRequestPlan?;`
- `export fn formatUpstreamHttp1Request(method: AppMethod, target_path: string, body: string, cfg: ProxyConfig, client_ip: string, out_buf: []u8): usize?;`
- `export fn proxyHook(ctx: &mut Context): bool;`
- `export fn createProxyPlugin(cfg: ProxyConfig, transport: ProxyTransportFn?): Plugin;`
- `export fn registerProxy(app: &mut App, cfg: ProxyConfig): &mut App;`
- `export fn registerProxyWithTransport(app: &mut App, cfg: ProxyConfig, transport: ProxyTransportFn): &mut App;`
