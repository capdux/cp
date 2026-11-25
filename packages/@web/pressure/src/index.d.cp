// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class PressureConfig {
    max_body_bytes: usize,
    max_inflight: u32,
    retry_after_secs: string,
    health_path: string,
    ready_path: string,
}

export fn pressureSetReady(ready: bool);
export fn pressureSetInflightForTest(n: u32);
export fn pressureShedCount(): u64;
export fn pressureResetStats();
export fn pressStrEql(a: string, b: string): bool;
export fn pressureOnRequest(ctx: &mut Context): bool;
export fn pressureOnAfter(ctx: &mut Context, resp: &mut Response);
export fn handleHealthz(ctx: &mut Context): Response;
export fn handleReadyz(ctx: &mut Context): Response;
export fn createPressurePlugin(cfg: PressureConfig): Plugin;
export fn registerPressure(app: &mut App, cfg: PressureConfig): &mut App;
