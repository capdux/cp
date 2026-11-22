// Source: cp/cap/test/test_health_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:health & cap.health

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON } from "cap:json";
import cap from "cap";
import {
    HealthError,
    ProbeKind,
    HealthStatus,
    CheckOutcome,
    ProbeReport,
    HealthRegistry,
    ShutdownCoordinator,
} from "cap:time";

struct ProbeState {
    db_status: HealthStatus = .Up;
    cache_status: HealthStatus = .Up;
    mem_status: HealthStatus = .Up;
    db_calls: u64 = 0;
}

fn checkDb(ctx_ptr: *anyopaque): CheckOutcome {
    unsafe {
        let st = ctx_ptr as *ProbeState!;
        st.db_calls += 1;
        switch (st.db_status) {
            .Up => return CheckOutcome{ status: .Up, message: "postgres \"primary\" ok\nready", latency_ms: 3 },
            .Degraded => return CheckOutcome{ status: .Degraded, message: "replication lag", latency_ms: 45 },
            .Down => return CheckOutcome{ status: .Down, message: "connection refused", latency_ms: 120 },
        }
    }
}

fn checkCache(ctx_ptr: *anyopaque): CheckOutcome {
    unsafe {
        const st = ctx_ptr as *ProbeState!;
        return CheckOutcome{ status: st.cache_status, message: "redis", latency_ms: 2 };
    }
}

fn checkMem(ctx_ptr: *anyopaque): CheckOutcome {
    unsafe {
        const st = ctx_ptr as *ProbeState!;
        return CheckOutcome{ status: st.mem_status, message: "heap ok", latency_ms: 1 };
    }
}

struct ShutdownTrace {
    steps: [8]u8 = [0; 8];
    len: usize = 0;
    http_runs: u64 = 0;
    worker_runs: u64 = 0;
    metrics_runs: u64 = 0;
    db_runs: u64 = 0;
}

fn hookStopHttp(ctx_ptr: *anyopaque) {
    unsafe {
        let tr = ctx_ptr as *ShutdownTrace!;
        tr.steps[tr.len] = 'H' as u8;
        tr.len += 1;
        tr.http_runs += 1;
    }
}

fn hookFlushWorker(ctx_ptr: *anyopaque) {
    unsafe {
        let tr = ctx_ptr as *ShutdownTrace!;
        tr.steps[tr.len] = 'W' as u8;
        tr.len += 1;
        tr.worker_runs += 1;
    }
}

fn hookFlushMetrics(ctx_ptr: *anyopaque) {
    unsafe {
        let tr = ctx_ptr as *ShutdownTrace!;
        tr.steps[tr.len] = 'M' as u8;
        tr.len += 1;
        tr.metrics_runs += 1;
    }
}

fn hookCloseDb(ctx_ptr: *anyopaque) {
    unsafe {
        let tr = ctx_ptr as *ShutdownTrace!;
        tr.steps[tr.len] = 'D' as u8;
        tr.len += 1;
        tr.db_runs += 1;
    }
}

fn testAggregationAndJson(): ! {
    const alloc = std.heap.page_allocator;
    let reg = HealthRegistry.init(alloc);
    defer reg.deinit();

    let st = ProbeState{};
    unsafe {
        const ctx = *st as *anyopaque!;
        reg.registerCheck("postgres", ProbeKind.Readiness, true, checkDb, ctx)?;
        reg.registerCheck("redis", ProbeKind.Readiness, false, checkCache, ctx)?;
        reg.registerCheck("memory", ProbeKind.All, true, checkMem, ctx)?;
    }

    // 1. All Up -> Up (200), 3 passing checks, RFC 8259 JSON valid
    let r1 = reg.evaluate(ProbeKind.Readiness)?;
    defer r1.deinit();
    expect_eq(r1.overall == HealthStatus.Up, true);
    expect_eq(r1.http_status, 200 as u16);
    expect_eq(r1.passing_count, 3 as usize);
    expect_eq(r1.degraded_count, 0 as usize);
    expect_eq(r1.failing_count, 0 as usize);

    const j1 = try JSON.parse(alloc, r1.details_json);
    expect_eq(j1.isObject(), true);
    expect_eq(std.mem.eql(u8, j1.get("status").?.asString().?, "up"), true);
    expect_eq(j1.get("checks").?.len(), 3 as usize);

    // 2. Non-critical Down -> Degraded (200)
    st.cache_status = .Down;
    let r2 = reg.evaluate(ProbeKind.Readiness)?;
    defer r2.deinit();
    expect_eq(r2.overall == HealthStatus.Degraded, true);
    expect_eq(r2.http_status, 200 as u16);
    expect_eq(r2.passing_count, 2 as usize);
    expect_eq(r2.failing_count, 1 as usize);

    const j2 = try JSON.parse(alloc, r2.details_json);
    expect_eq(std.mem.eql(u8, j2.get("status").?.asString().?, "degraded"), true);

    // 3. Critical Degraded (non-critical Up) -> Degraded (200)
    st.cache_status = .Up;
    st.db_status = .Degraded;
    let r3 = reg.evaluate(ProbeKind.Readiness)?;
    defer r3.deinit();
    expect_eq(r3.overall == HealthStatus.Degraded, true);
    expect_eq(r3.http_status, 200 as u16);
    expect_eq(r3.degraded_count, 1 as usize);

    // 4. Critical Down -> Down (503)
    st.db_status = .Down;
    let r4 = reg.evaluate(ProbeKind.Readiness)?;
    defer r4.deinit();
    expect_eq(r4.overall == HealthStatus.Down, true);
    expect_eq(r4.http_status, 503 as u16);
    expect_eq(r4.failing_count, 1 as usize);

    const j4 = try JSON.parse(alloc, r4.details_json);
    expect_eq(std.mem.eql(u8, j4.get("status").?.asString().?, "down"), true);
}

fn testDrainingIsolation(): ! {
    const alloc = std.heap.page_allocator;
    let reg = HealthRegistry.init(alloc);
    defer reg.deinit();

    let st = ProbeState{};
    unsafe {
        const ctx = *st as *anyopaque!;
        reg.registerCheck("postgres", ProbeKind.Readiness, true, checkDb, ctx)?;
        reg.registerCheck("liveness_mem", ProbeKind.Liveness, true, checkMem, ctx)?;
    }

    // Before draining: both Readiness and Liveness are 200
    let r_before = reg.evaluate(ProbeKind.Readiness)?;
    defer r_before.deinit();
    expect_eq(r_before.http_status, 200 as u16);

    let l_before = reg.evaluate(ProbeKind.Liveness)?;
    defer l_before.deinit();
    expect_eq(l_before.http_status, 200 as u16);

    // Enter draining mode: Readiness -> Down (503), Liveness -> still Up (200)!
    reg.setDraining(true);
    expect_eq(reg.isDraining(), true);

    let r_drain = reg.evaluate(ProbeKind.Readiness)?;
    defer r_drain.deinit();
    expect_eq(r_drain.overall == HealthStatus.Down, true);
    expect_eq(r_drain.http_status, 503 as u16);

    const j_drain = try JSON.parse(alloc, r_drain.details_json);
    expect_eq(j_drain.get("draining").?.asBool().?, true);

    let l_drain = reg.evaluate(ProbeKind.Liveness)?;
    defer l_drain.deinit();
    expect_eq(l_drain.overall == HealthStatus.Up, true);
    expect_eq(l_drain.http_status, 200 as u16);

    // Exit draining mode -> Readiness recovers to 200
    reg.setDraining(false);
    let r_after = reg.evaluate(ProbeKind.Readiness)?;
    defer r_after.deinit();
    expect_eq(r_after.overall == HealthStatus.Up, true);
    expect_eq(r_after.http_status, 200 as u16);
}

fn testDuplicateCheckAndDynamicEvaluation(): ! {
    const alloc = std.heap.page_allocator;
    let reg = HealthRegistry.init(alloc);
    defer reg.deinit();

    let st = ProbeState{};
    unsafe {
        const ctx = *st as *anyopaque!;
        reg.registerCheck("db", ProbeKind.Startup, true, checkDb, ctx)?;

        let got_dup = false;
        * = reg.registerCheck("db", ProbeKind.Readiness, true, checkDb, ctx) catch e => {
            if (std.mem.eql(u8, @errorName(e), "DuplicateCheck")) {
                got_dup = true;
            }
        };
        expect_eq(got_dup, true);
        expect_eq(reg.checkCount(), 1 as usize);
    }

    // Dynamic state transitions on Startup probe
    st.db_status = .Down;
    let r_down = reg.evaluate(ProbeKind.Startup)?;
    defer r_down.deinit();
    expect_eq(r_down.http_status, 503 as u16);

    st.db_status = .Up;
    let r_up = reg.evaluate(ProbeKind.Startup)?;
    defer r_up.deinit();
    expect_eq(r_up.http_status, 200 as u16);
    expect_eq(st.db_calls, 2 as u64);

    // Unregister and verify CheckNotFound
    reg.unregisterCheck("db")?;
    expect_eq(reg.checkCount(), 0 as usize);

    let got_not_found = false;
    * = reg.unregisterCheck("db") catch e => {
        if (std.mem.eql(u8, @errorName(e), "CheckNotFound")) {
            got_not_found = true;
        }
    };
    expect_eq(got_not_found, true);
}

fn testShutdownCoordinatorPriorityAndIdempotency(): ! {
    const alloc = std.heap.page_allocator;
    let coord = ShutdownCoordinator.init(alloc);
    defer coord.deinit();

    let tr = ShutdownTrace{};
    unsafe {
        const ctx = *tr as *anyopaque!;
        // Register out of priority order to verify descending sort + FIFO tie-breaker
        coord.registerHook("db_pool", 10, hookCloseDb, ctx)?;
        coord.registerHook("worker_flush", 50, hookFlushWorker, ctx)?;
        coord.registerHook("http_ingress", 100, hookStopHttp, ctx)?;
        coord.registerHook("metrics_flush", 50, hookFlushMetrics, ctx)?;

        let got_dup_hook = false;
        * = coord.registerHook("db_pool", 20, hookCloseDb, ctx) catch e => {
            if (std.mem.eql(u8, @errorName(e), "DuplicateHook")) {
                got_dup_hook = true;
            }
        };
        expect_eq(got_dup_hook, true);
    }

    expect_eq(coord.hookCount(), 4 as usize);
    expect_eq(coord.isShuttingDown(), false);

    // First trigger: runs all 4 hooks in order H (100) -> W (50, seq 1) -> M (50, seq 3) -> D (10)
    const ran1 = coord.triggerShutdown();
    expect_eq(ran1, 4 as usize);
    expect_eq(coord.isShuttingDown(), true);
    expect_eq(std.mem.eql(u8, tr.steps[0..tr.len], "HWMD"), true);

    const order = coord.executedOrder();
    expect_eq(order.len, 4 as usize);
    expect_eq(std.mem.eql(u8, order[0], "http_ingress"), true);
    expect_eq(std.mem.eql(u8, order[1], "worker_flush"), true);
    expect_eq(std.mem.eql(u8, order[2], "metrics_flush"), true);
    expect_eq(std.mem.eql(u8, order[3], "db_pool"), true);

    // Second & third triggers: strictly idempotent, return 0 and never re-run hooks
    const ran2 = coord.triggerShutdown();
    const ran3 = coord.triggerShutdown();
    expect_eq(ran2, 0 as usize);
    expect_eq(ran3, 0 as usize);
    expect_eq(tr.http_runs, 1 as u64);
    expect_eq(tr.worker_runs, 1 as u64);
    expect_eq(tr.metrics_runs, 1 as u64);
    expect_eq(tr.db_runs, 1 as u64);

    // Registering after shutdown must fail with AlreadyShuttingDown
    unsafe {
        const ctx = *tr as *anyopaque!;
        let got_already = false;
        * = coord.registerHook("late_hook", 1, hookCloseDb, ctx) catch e => {
            if (std.mem.eql(u8, @errorName(e), "AlreadyShuttingDown")) {
                got_already = true;
            }
        };
        expect_eq(got_already, true);
    }
}

fn testCapHealthDynamicMounting(): ! {
    const alloc = std.heap.page_allocator;
    let reg = cap.time.HealthRegistry.init(alloc);
    defer reg.deinit();

    let st = ProbeState{};
    unsafe {
        const ctx = *st as *anyopaque!;
        reg.registerCheck("dyn_db", cap.time.ProbeKind.Readiness, true, checkDb, ctx)?;
    }
    let report = reg.evaluate(cap.time.ProbeKind.Readiness)?;
    defer report.deinit();
    expect_eq(report.overall == cap.time.HealthStatus.Up, true);
    expect_eq(report.http_status, 200 as u16);

    let coord = cap.time.ShutdownCoordinator.init(alloc);
    defer coord.deinit();
    let tr = ShutdownTrace{};
    unsafe {
        const ctx = *tr as *anyopaque!;
        coord.registerHook("dyn_http", 100, hookStopHttp, ctx)?;
    }
    expect_eq(coord.triggerShutdown(), 1 as usize);
    expect_eq(tr.http_runs, 1 as u64);
}

pub fn main() {
    test("1. HealthRegistry critical vs non-critical aggregation & RFC 8259 JSON", () => {
        testAggregationAndJson() catch unreachable;
    });
    test("2. setDraining(true) Readiness 503 vs Liveness 200 isolation", () => {
        testDrainingIsolation() catch unreachable;
    });
    test("3. DuplicateCheck, unregisterCheck & dynamic re-evaluate", () => {
        testDuplicateCheckAndDynamicEvaluation() catch unreachable;
    });
    test("4. ShutdownCoordinator priority descending order & strict idempotency", () => {
        testShutdownCoordinatorPriorityAndIdempotency() catch unreachable;
    });
    test("5. import cap from \"cap\"; cap.health Dynamic Lazy Mounting", () => {
        testCapHealthDynamicMounting() catch unreachable;
    });
}
