[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/proxy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official zero-heap HTTP reverse proxy plugin for cap:web (prefix stripping, traversal guard, HTTP/1.1 formatter, pluggable transport)

## 安装与导入

```bash
cap add @web/proxy
```

```cp
import { pxAsciiLower } from "@web/proxy";
```

## 核心导出 API

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
