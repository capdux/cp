// Unit tests for @web/cron
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    CronTime,
    CronExpr,
    CronTaskFn,
    CronScheduler,
    CronConfig,
    parseCronExpr,
    cronMatches,
    cronGlobalScheduler,
    resetCronScheduler,
    registerCron,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEql(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

let g_job_a_ticks: u64 = 0;
let g_job_b_ticks: u64 = 0;

fn jobAHandler() {
    g_job_a_ticks += 1;
}

fn jobBHandler() {
    g_job_b_ticks += 1;
}

pub fn main() {
    // 1. Parse & match standard 5-field expressions: *, exact, */step, range, list
    const e_step = parseCronExpr("*/15 9-17 * * 1-5") orelse {
        assertTrue(false, "parse */15 9-17 * * 1-5");
        return;
    };
    assertTrue(
        cronMatches(&e_step, CronTime{ minute: 0, hour: 9, day: 10, month: 3, weekday: 1 }),
        "matches 09:00 Mon",
    );
    assertTrue(
        cronMatches(&e_step, CronTime{ minute: 45, hour: 17, day: 10, month: 3, weekday: 5 }),
        "matches 17:45 Fri",
    );
    assertTrue(
        !cronMatches(&e_step, CronTime{ minute: 10, hour: 9, day: 10, month: 3, weekday: 1 }),
        "rejects minute 10 for */15",
    );
    assertTrue(
        !cronMatches(&e_step, CronTime{ minute: 0, hour: 18, day: 10, month: 3, weekday: 1 }),
        "rejects hour 18 for 9-17",
    );
    assertTrue(
        !cronMatches(&e_step, CronTime{ minute: 0, hour: 9, day: 10, month: 3, weekday: 0 }),
        "rejects Sunday for 1-5",
    );

    // 2. List & exact values + weekday 7 normalized to Sunday (0)
    const e_list = parseCronExpr("0,30 6 1,15 * 7") orelse {
        assertTrue(false, "parse list expr");
        return;
    };
    assertTrue(
        cronMatches(&e_list, CronTime{ minute: 30, hour: 6, day: 15, month: 11, weekday: 0 }),
        "list & weekday 7 -> Sunday match",
    );
    assertTrue(
        !cronMatches(&e_list, CronTime{ minute: 15, hour: 6, day: 15, month: 11, weekday: 0 }),
        "list rejects non-listed minute",
    );

    // 3. Invalid expressions rejected
    assertTrue(parseCronExpr("* * * *") == null, "rejects 4 fields");
    assertTrue(parseCronExpr("60 * * * *") == null, "rejects minute 60");
    assertTrue(parseCronExpr("*/0 * * * *") == null, "rejects step 0");
    assertTrue(parseCronExpr("10-5 * * * *") == null, "rejects inverted range");

    // 4. CronScheduler addJob / tick / pauseJob / resumeJob / jobRunCount
    resetCronScheduler();
    g_job_a_ticks = 0;
    g_job_b_ticks = 0;

    const sched = cronGlobalScheduler();
    assertTrue(sched.addJob("heartbeat", "*/5 * * * *", jobAHandler), "add heartbeat job");
    assertTrue(sched.addJob("nightly", "0 0 * * *", jobBHandler), "add nightly job");
    assertTrue(!sched.addJob("heartbeat", "* * * * *", jobAHandler), "reject duplicate job name");
    assertTrue(!sched.addJob("bad", "bad expr", jobAHandler), "reject invalid expr");

    // Tick at 00:00 -> both jobs fire
    const fired1 = sched.tick(CronTime{ minute: 0, hour: 0, day: 1, month: 1, weekday: 1 });
    assertTrue(fired1 == 2, "00:00 fires 2 jobs");
    assertTrue(g_job_a_ticks == 1 && sched.jobRunCount("heartbeat") == 1, "heartbeat ran once");
    assertTrue(g_job_b_ticks == 1 && sched.jobRunCount("nightly") == 1, "nightly ran once");

    // Pause heartbeat, tick at 00:05 -> 0 jobs fire
    assertTrue(sched.pauseJob("heartbeat"), "pause heartbeat");
    const fired2 = sched.tick(CronTime{ minute: 5, hour: 0, day: 1, month: 1, weekday: 1 });
    assertTrue(fired2 == 0, "paused heartbeat does not fire");
    assertTrue(sched.jobRunCount("heartbeat") == 1, "heartbeat count unchanged while paused");

    // Resume heartbeat, tick at 00:10 -> 1 job fires
    assertTrue(sched.resumeJob("heartbeat"), "resume heartbeat");
    const fired3 = sched.tick(CronTime{ minute: 10, hour: 0, day: 1, month: 1, weekday: 1 });
    assertTrue(fired3 == 1, "resumed heartbeat fires");
    assertTrue(sched.jobRunCount("heartbeat") == 2, "heartbeat count incremented to 2");

    // 5. App integration: GET /_cron returns JSON status
    let app = App.init();
    * = registerCron(*app, CronConfig{ status_path: "/_cron" });
    const resp = app.dispatch(Request.init(AppMethod.GET, "/_cron"));
    assertTrue(resp.status == 200, "GET /_cron returns 200");
    assertTrue(strContains(resp.body, "\"count\":2"), "status JSON contains count:2");
    assertTrue(strContains(resp.body, "\"name\":\"heartbeat\""), "status JSON contains heartbeat");
    assertTrue(strContains(resp.body, "\"runs\":2"), "status JSON contains runs:2");
    assertTrue(strContains(resp.body, "\"name\":\"nightly\""), "status JSON contains nightly");

    std.debug.print("PASS: @web/cron all tests passed!\n", .{});
}
