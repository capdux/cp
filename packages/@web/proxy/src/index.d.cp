// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class ProxyConfig {
    prefix: string,
    upstream_host: string,
    upstream_port: u16,
    strip_prefix: bool,
    add_forwarded_headers: bool,
}

export class ProxyRequestPlan {
    target_path: u8[256],
    target_path_len: usize,
    host_header: u8[128],
    host_header_len: usize,
    forwarded_for: string,
    forwarded_proto: string,
    fn targetPathSlice(): string;
    fn hostHeaderSlice(): string;
}

export fn pxAsciiLower(c: u8): u8;
export fn pxWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn pxWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn pxWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn pxHasControlChars(s: string): bool;
export fn pxHasTraversal(path: string): bool;
export fn pxTrimTrailingSlash(s: string): string;
export fn pxMatchesPrefix(req_path: string, raw_prefix: string): bool;
export fn pxDefaultTransport(cfg: &ProxyConfig, ctx: &mut Context, target_path: string): Response;
export fn buildProxyTargetPath(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): ?usize;
export fn buildProxyRequestPlan(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ?ProxyRequestPlan;
export fn formatUpstreamHttp1Request(method: AppMethod, target_path: string, body: string, cfg: ProxyConfig, client_ip: string, out_buf: []u8): ?usize;
export fn proxyHook(ctx: &mut Context): bool;
export fn createProxyPlugin(cfg: ProxyConfig, transport: ?ProxyTransportFn): Plugin;
export fn registerProxy(app: &mut App, cfg: ProxyConfig): &mut App;
export fn registerProxyWithTransport(app: &mut App, cfg: ProxyConfig, transport: ProxyTransportFn): &mut App;
