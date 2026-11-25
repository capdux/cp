// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class LoggerConfig {
    request_id_header: string,
    metrics_path: string,
    enable_trace: bool,
}

export class MetricsSnapshot {
    requests_total: u64,
    responses_2xx_total: u64,
    responses_4xx_total: u64,
    responses_5xx_total: u64,
}

export fn logWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn logWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn logHexDigitLower(n: u8): u8;
export fn logWriteU64Decimal(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn logWriteU64HexFixed16(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn logWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn formatAccessLogJson(method: string, path: string, status: u16, req_id: string, out_buf: []u8): usize?;
export fn formatTraceparent(seq: u64, out_buf: []u8): usize?;
export fn formatOpenMetrics(snap: MetricsSnapshot, out_buf: []u8): usize?;
export fn resetState();
export fn getMetrics(): MetricsSnapshot;
export fn lastAccessLog(): string;
export fn onRequestHook(ctx: &mut Context): bool;
export fn onAfterHook(ctx: &mut Context, resp: &mut Response);
export fn metricsHandler(ctx: &mut Context): Response;
export fn createLoggerPlugin(cfg: LoggerConfig): Plugin;
export fn registerLogger(app: &mut App, cfg: LoggerConfig): &mut App;
