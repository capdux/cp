// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export enum CircuitState {
    Closed,
    Open,
    HalfOpen
}

export class CircuitBreakerConfig {
    failure_threshold: u32,
    reset_timeout_secs: u64,
    retry_after_secs: string,
}

export fn cbNowSec(): u64;
export fn cbState(): CircuitState;
export fn cbFailureCount(): u32;
export fn cbSetTestClock(now_sec: u64);
export fn cbReset();
export fn cbBeforeHandle(ctx: &mut Context): bool;
export fn cbAfterHandle(ctx: &mut Context, resp: &mut Response);
export fn cbCreatePlugin(cfg: CircuitBreakerConfig): Plugin;
export fn registerCircuitBreaker(app: &mut App, cfg: CircuitBreakerConfig): &mut App;
