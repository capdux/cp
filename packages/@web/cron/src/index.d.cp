// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class CronTime {
    minute: u8,
    hour: u8,
    day: u8,
    month: u8,
    weekday: u8,
}

export class CronExpr {
    minutes: u64,
    hours: u64,
    days: u64,
    months: u64,
    weekdays: u64,
}

export class CronConfig {
    status_path: string,
}

export class CronJob {
    used: bool,
    name_buf: u8[CRON_MAX_NAME_LEN],
    name_len: usize,
    expr_buf: u8[CRON_MAX_EXPR_LEN],
    expr_len: usize,
    expr: CronExpr,
    handler: ?CronTaskFn,
    paused: bool,
    run_count: u64,
    fn nameSlice(): string;
    fn exprSlice(): string;
    fn matchesName(name: string): bool;
}

export fn cronParseU8(s: string): ?u8;
export fn cronParseField(field: string, min_val: u8, max_val: u8, is_weekday: bool): ?u64;
export fn cronWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn cronWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
export fn parseCronExpr(expr: string): ?CronExpr;
export fn cronMatches(expr: &CronExpr, t: CronTime): bool;
export class CronScheduler {
    jobs: CronJob[CRON_MAX_JOBS],
    job_count: usize,
    fn init(): CronScheduler;
    fn reset(_: any);
    fn addJob(_: any, name: string, expr: string, handler: CronTaskFn): bool;
    fn pauseJob(_: any, name: string): bool;
    fn resumeJob(_: any, name: string): bool;
    fn tick(_: any, t: CronTime): usize;
    fn jobRunCount(name: string): u64;
    fn formatStatusJson(out_buf: []u8): ?usize;
}

export fn cronGlobalScheduler(): &mut CronScheduler;
export fn resetCronScheduler();
export fn cronStatusHandler(ctx: &mut Context): Response;
export fn createCronPlugin(cfg: CronConfig): Plugin;
export fn registerCron(app: &mut App, cfg: CronConfig): &mut App;
