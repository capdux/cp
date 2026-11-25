// Origin: Official @web/cron Plugin (Pure-CP Zero-Heap 5-Field Cron Scheduler) | Created: 2026-11-25
// ============================================================================
// @web/cron — Zero-Heap 5-Field Cron Expression Parser & Scheduler for cap:web
//   - Inspired by Elysia `@elysiajs/cron`
//   - Standard 5-field Cron syntax: `*`, exact `5`, step `*/15`, range `1-5`, list `0,30`, `1-10/2`
//   - O(1) 64-bit bitmap matching + fixed 16-slot scheduler with pause/resume & run counters
//   - Built-in `GET /_cron` JSON status endpoint
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const CRON_MAX_JOBS: usize = 16;
pub const CRON_MAX_NAME_LEN: usize = 64;
pub const CRON_MAX_EXPR_LEN: usize = 64;

pub struct CronTime {
    minute: u8 = 0;
    hour: u8 = 0;
    day: u8 = 1;
    month: u8 = 1;
    weekday: u8 = 0;
}

pub struct CronExpr {
    minutes: u64 = 0;
    hours: u64 = 0;
    days: u64 = 0;
    months: u64 = 0;
    weekdays: u64 = 0;
}

pub type CronTaskFn = *const fn (): void;

pub struct CronConfig {
    status_path: string = "/_cron";
}

pub struct CronJob {
    used: bool = false;
    name_buf: [CRON_MAX_NAME_LEN]u8 = [0; CRON_MAX_NAME_LEN];
    name_len: usize = 0;
    expr_buf: [CRON_MAX_EXPR_LEN]u8 = [0; CRON_MAX_EXPR_LEN];
    expr_len: usize = 0;
    expr: CronExpr = CronExpr{};
    handler: CronTaskFn? = null;
    paused: bool = false;
    run_count: u64 = 0;

    fn nameSlice(): string {
        return _.name_buf[0.._.name_len];
    }

    fn exprSlice(): string {
        return _.expr_buf[0.._.expr_len];
    }

    fn matchesName(name: string): bool {
        if (!_.used || _.name_len != name.len) return false;
        let i: usize = 0;
        while (i < name.len) : (i += 1) {
            if (_.name_buf[i] != name[i]) return false;
        }
        return true;
    }
}

// ============================================================================
// Internal Prefix-Scoped Helpers (cron*)
// ============================================================================

fn cronParseU8(s: string): u8? {
    if (s.len == 0 || s.len > 3) return null;
    let val: u16 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u16, c - '0');
        if (val > 255) return null;
    }
    return @truncate(val);
}

fn cronParseField(field: string, min_val: u8, max_val: u8, is_weekday: bool): u64? {
    if (field.len == 0) return null;
    let mask: u64 = 0;
    let pos: usize = 0;

    while (pos <= field.len) {
        let end = pos;
        while (end < field.len && field[end] != ',') : (end += 1) {}
        const item = field[pos..end];
        if (item.len == 0) return null;

        // Optional `/step`
        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < item.len) : (i += 1) {
            if (item[i] == '/') {
                if (slash_idx != null) return null;
                slash_idx = i;
            }
        }

        let base_part: string = item;
        let step: u8 = 1;
        if (sidx := slash_idx) {
            base_part = item[0..sidx];
            const step_str = item[sidx + 1 ..];
            const parsed_step = cronParseU8(step_str) orelse return null;
            if (parsed_step == 0) return null;
            step = parsed_step;
        }

        let range_start: u8 = min_val;
        let range_end: u8 = max_val;

        if (base_part.len == 1 && base_part[0] == '*') {
            range_start = min_val;
            range_end = max_val;
        } else {
            let dash_idx: usize? = null;
            i = 0;
            while (i < base_part.len) : (i += 1) {
                if (base_part[i] == '-') {
                    if (dash_idx != null) return null;
                    dash_idx = i;
                }
            }
            if (didx := dash_idx) {
                const a = cronParseU8(base_part[0..didx]) orelse return null;
                const b = cronParseU8(base_part[didx + 1 ..]) orelse return null;
                const max_allowed: u8 = if (is_weekday) 7 else max_val;
                if (a < min_val || b > max_allowed || a > b) return null;
                range_start = a;
                range_end = b;
            } else {
                const single = cronParseU8(base_part) orelse return null;
                const max_allowed: u8 = if (is_weekday) 7 else max_val;
                if (single < min_val || single > max_allowed) return null;
                range_start = single;
                range_end = if (slash_idx != null) max_val else single;
            }
        }

        let v: u16 = @as(u16, range_start);
        const end_u16: u16 = @as(u16, range_end);
        const step_u16: u16 = @as(u16, step);
        while (v <= end_u16) : (v += step_u16) {
            const bit_idx: u6 = if (is_weekday && v == 7) 0 else @truncate(v);
            mask |= (@as(u64, 1) << bit_idx);
        }

        if (end >= field.len) break;
        pos = end + 1;
    }

    if (mask == 0) return null;
    return mask;
}

fn cronWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn cronWriteU64(out_buf: []u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        if (pos.* >= out_buf.len) return false;
        out_buf[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > out_buf.len) return false;
    while (n > 0) {
        n -= 1;
        out_buf[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

// ============================================================================
// Public Cron Expression API
// ============================================================================

/// Parse a standard 5-field Cron expression (`min hour day month weekday`).
pub fn parseCronExpr(expr: string): CronExpr? {
    let fields: [5]string = ["", "", "", "", ""];
    let field_count: usize = 0;
    let i: usize = 0;
    while (i < expr.len) {
        while (i < expr.len && (expr[i] == ' ' || expr[i] == '\t')) : (i += 1) {}
        if (i >= expr.len) break;
        const start = i;
        while (i < expr.len && expr[i] != ' ' && expr[i] != '\t') : (i += 1) {}
        if (field_count >= 5) return null;
        fields[field_count] = expr[start..i];
        field_count += 1;
    }
    if (field_count != 5) return null;

    const mins = cronParseField(fields[0], 0, 59, false) orelse return null;
    const hrs = cronParseField(fields[1], 0, 23, false) orelse return null;
    const days = cronParseField(fields[2], 1, 31, false) orelse return null;
    const mons = cronParseField(fields[3], 1, 12, false) orelse return null;
    const wds = cronParseField(fields[4], 0, 6, true) orelse return null;

    return CronExpr{
        minutes: mins,
        hours: hrs,
        days: days,
        months: mons,
        weekdays: wds,
    };
}

/// Evaluate whether `t` matches the parsed `CronExpr` in O(1) time.
pub fn cronMatches(expr: *const CronExpr, t: CronTime): bool {
    if (t.minute > 59 || t.hour > 23 || t.day < 1 || t.day > 31 || t.month < 1 || t.month > 12 || t.weekday > 7) {
        return false;
    }
    const wd: u6 = if (t.weekday == 7) 0 else @truncate(t.weekday);
    const m_ok = (expr.minutes & (@as(u64, 1) << @as(u6, @truncate(t.minute)))) != 0;
    const h_ok = (expr.hours & (@as(u64, 1) << @as(u6, @truncate(t.hour)))) != 0;
    const d_ok = (expr.days & (@as(u64, 1) << @as(u6, @truncate(t.day)))) != 0;
    const mo_ok = (expr.months & (@as(u64, 1) << @as(u6, @truncate(t.month)))) != 0;
    const w_ok = (expr.weekdays & (@as(u64, 1) << wd)) != 0;
    return m_ok && h_ok && d_ok && mo_ok && w_ok;
}

// ============================================================================
// Public CronScheduler & Plugin Integration
// ============================================================================

pub struct CronScheduler {
    jobs: [CRON_MAX_JOBS]CronJob = [CronJob{}; CRON_MAX_JOBS];
    job_count: usize = 0;

    pub static fn init(): CronScheduler {
        return CronScheduler{
            jobs: [CronJob{}; CRON_MAX_JOBS],
            job_count: 0,
        };
    }

    pub fn reset(*_) {
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            _.jobs[i] = CronJob{};
        }
        _.job_count = 0;
    }

    pub fn addJob(*_, name: string, expr: string, handler: CronTaskFn): bool {
        if (name.len == 0 || name.len > CRON_MAX_NAME_LEN) return false;
        if (expr.len == 0 || expr.len > CRON_MAX_EXPR_LEN) return false;
        const parsed = parseCronExpr(expr) orelse return false;

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].matchesName(name)) return false;
            if (!_.jobs[i].used && free_idx == null) {
                free_idx = i;
            }
        }
        const idx = free_idx orelse return false;

        let job = CronJob{};
        job.used = true;
        i = 0;
        while (i < name.len) : (i += 1) {
            job.name_buf[i] = name[i];
        }
        job.name_len = name.len;
        i = 0;
        while (i < expr.len) : (i += 1) {
            job.expr_buf[i] = expr[i];
        }
        job.expr_len = expr.len;
        job.expr = parsed;
        job.handler = handler;
        job.paused = false;
        job.run_count = 0;

        _.jobs[idx] = job;
        _.job_count += 1;
        return true;
    }

    pub fn pauseJob(*_, name: string): bool {
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].matchesName(name)) {
                _.jobs[i].paused = true;
                return true;
            }
        }
        return false;
    }

    pub fn resumeJob(*_, name: string): bool {
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].matchesName(name)) {
                _.jobs[i].paused = false;
                return true;
            }
        }
        return false;
    }

    pub fn tick(*_, t: CronTime): usize {
        let fired: usize = 0;
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].used && !_.jobs[i].paused) {
                if (cronMatches(&_.jobs[i].expr, t)) {
                    if (h := _.jobs[i].handler) {
                        h();
                    }
                    _.jobs[i].run_count += 1;
                    fired += 1;
                }
            }
        }
        return fired;
    }

    pub fn jobRunCount(name: string): u64 {
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].matchesName(name)) {
                return _.jobs[i].run_count;
            }
        }
        return 0;
    }

    pub fn formatStatusJson(out_buf: []u8): usize? {
        let pos: usize = 0;
        if (!cronWriteRaw(out_buf, *pos, "{\"count\":")) return null;
        if (!cronWriteU64(out_buf, *pos, @as(u64, _.job_count))) return null;
        if (!cronWriteRaw(out_buf, *pos, ",\"jobs\":[")) return null;
        let first = true;
        let i: usize = 0;
        while (i < CRON_MAX_JOBS) : (i += 1) {
            if (_.jobs[i].used) {
                if (!first) {
                    if (!cronWriteRaw(out_buf, *pos, ",")) return null;
                }
                first = false;
                if (!cronWriteRaw(out_buf, *pos, "{\"name\":\"")) return null;
                if (!cronWriteRaw(out_buf, *pos, _.jobs[i].nameSlice())) return null;
                if (!cronWriteRaw(out_buf, *pos, "\",\"expr\":\"")) return null;
                if (!cronWriteRaw(out_buf, *pos, _.jobs[i].exprSlice())) return null;
                if (!cronWriteRaw(out_buf, *pos, "\",\"paused\":")) return null;
                if (!cronWriteRaw(out_buf, *pos, if (_.jobs[i].paused) "true" else "false")) return null;
                if (!cronWriteRaw(out_buf, *pos, ",\"runs\":")) return null;
                if (!cronWriteU64(out_buf, *pos, _.jobs[i].run_count)) return null;
                if (!cronWriteRaw(out_buf, *pos, "}")) return null;
            }
        }
        if (!cronWriteRaw(out_buf, *pos, "]}")) return null;
        return pos;
    }
}

let g_cron_scheduler: CronScheduler = CronScheduler{};
let g_cron_status_buf: [4096]u8 = [0; 4096];

/// Access the global `@web/cron` scheduler instance.
pub fn cronGlobalScheduler(): *CronScheduler {
    return *g_cron_scheduler;
}

/// Reset the global `@web/cron` scheduler (useful between test cases).
pub fn resetCronScheduler() {
    g_cron_scheduler.reset();
}

pub fn cronStatusHandler(ctx: *Context): Response {
    * = ctx;
    const len = g_cron_scheduler.formatStatusJson(g_cron_status_buf[0..]) orelse {
        return Response.json(500, "{\"error\":\"Cron status buffer overflow\"}");
    };
    return Response.json(200, g_cron_status_buf[0..len]);
}

/// Create a composable Elysia-style `Plugin` for `@web/cron`.
pub fn createCronPlugin(cfg: CronConfig): Plugin {
    const path = if (cfg.status_path.len > 0) cfg.status_path else "/_cron";
    let p = Plugin.init("@web/cron");
    * = p.withScope(PluginScope.Global);
    * = p.get(path, cronStatusHandler);
    return p;
}

/// Register `@web/cron` directly onto `app` (mounts `GET /_cron` status JSON).
pub fn registerCron(app: *App, cfg: CronConfig): *App {
    const p = createCronPlugin(cfg);
    return app.usePlugin(&p);
}
