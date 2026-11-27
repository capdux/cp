[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/cron

Official zero-heap 5-field Cron scheduler plugin for cap:web (bitmap matcher, pause/resume, status endpoint)

| Metadata | Value |
| --- | --- |
| Package | `@web/cron` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/cron
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CronTime` | `class` | Fields: `minute: u8, hour: u8, day: u8, month: u8, weekday: u8` |
| `CronExpr` | `class` | Fields: `minutes: u64, hours: u64, days: u64, months: u64, weekdays: u64` |
| `CronConfig` | `class` | Fields: `status_path: string` |
| `CronJob` | `class` | Fields: `used: bool, name_buf: [CRON_MAX_NAME_LEN]u8, name_len: usize, expr_buf: [CRON_MAX_EXPR_LEN]u8, expr_len: usize, expr: CronExpr, +3 more` · Methods: `nameSlice, exprSlice, matchesName` |
| `CronScheduler` | `class` | Fields: `jobs: [CRON_MAX_JOBS]CronJob, job_count: usize` · Methods: `init, reset, addJob, pauseJob, resumeJob, tick, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cronParseU8` | `(s: string): u8?` | Invokes `cronParseU8` and returns `u8?`. |
| `cronParseField` | `(field: string, min_val: u8, max_val: u8, is_weekday: bool): u64?` | Invokes `cronParseField` and returns `u64?`. |
| `cronWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `cronWriteRaw` and returns `bool`. |
| `cronWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `cronWriteU64` and returns `bool`. |
| `parseCronExpr` | `(expr: string): CronExpr?` | Parses input via `parseCronExpr` and returns `CronExpr?`. |
| `cronMatches` | `(expr: &CronExpr, t: CronTime): bool` | Invokes `cronMatches` and returns `bool`. |
| `cronGlobalScheduler` | `(): &mut CronScheduler` | Invokes `cronGlobalScheduler` and returns `&mut CronScheduler`. |
| `resetCronScheduler` | `()` | Executes `resetCronScheduler` in-place. |
| `cronStatusHandler` | `(ctx: &mut Context): Response` | Invokes `cronStatusHandler` and returns `Response`. |
| `createCronPlugin` | `(cfg: CronConfig): Plugin` | Constructs/initializes an instance via `createCronPlugin`. |
| `registerCron` | `(app: &mut App, cfg: CronConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { parseCronExpr } from "@web/cron";

pub fn main() {
    let result = parseCronExpr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
