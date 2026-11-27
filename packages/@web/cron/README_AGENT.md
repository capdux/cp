# @web/cron — AI Agent Quick Ref

Official zero-heap 5-field Cron scheduler plugin for cap:web (bitmap matcher, pause/resume, status endpoint) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CronTime,
    CronExpr,
    CronConfig,
    CronJob,
    CronScheduler,
    cronParseU8,
    cronParseField,
    cronWriteRaw,
} from "@web/cron";
```

## Core Types & Constants

```cp
class CronTime { minute: u8, hour: u8, day: u8, month: u8, /* +1 fields */ }
class CronExpr { minutes: u64, hours: u64, days: u64, months: u64, /* +1 fields */ }
class CronConfig { status_path: string }
class CronJob { used: bool, name_buf: [CRON_MAX_NAME_LEN]u8, name_len: usize, expr_buf: [CRON_MAX_EXPR_LEN]u8, /* +5 fields */ } // methods: nameSlice(...) +2
class CronScheduler { jobs: [CRON_MAX_JOBS]CronJob, job_count: usize } // methods: init(...) +7
```

## API Signatures

```cp
fn cronParseU8(s: string): u8?;
fn cronParseField(field: string, min_val: u8, max_val: u8, is_weekday: bool): u64?;
fn cronWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn cronWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn parseCronExpr(expr: string): CronExpr?;
fn cronMatches(expr: &CronExpr, t: CronTime): bool;
fn cronGlobalScheduler(): &mut CronScheduler;
fn resetCronScheduler();
fn cronStatusHandler(ctx: &mut Context): Response;
fn createCronPlugin(cfg: CronConfig): Plugin;
fn registerCron(app: &mut App, cfg: CronConfig): &mut App;
```

## Minimal Usage

```cp
import { parseCronExpr } from "@web/cron";

pub fn main() {
    let result = parseCronExpr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/cron"`; install via `cap add @web/cron`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
