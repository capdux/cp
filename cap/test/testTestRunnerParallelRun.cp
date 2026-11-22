// Deep production regression tests for cap:test_runner parallelRun.cp
// Covers: effectiveWorkerCount & serial fallback, scale-up delay resolution (CLI vs env vs negative clamp),
// lexicographic locality sorting vs --randomize preservation and K-way contiguous partitioning,
// 9-tuple WorkerFileDelta diff + 36-byte LE wire roundtrip, WorkerArgvBuilder flags, and WorkerEnvPlan.

import { test, expect, expectEq } from "cap:test";
import {
    DEFAULT_PARALLEL_SCALE_UP_MS,
    WORKER_FILE_DELTA_BYTES,
    effectiveWorkerCount,
    shouldUseParallelPool,
    resolveScaleUpDelayMs,
    ParallelFilePartition,
    WorkerSummarySnapshot,
    WorkerFileDelta,
    WorkerCliOptions,
    WorkerArgvBuilder,
    WorkerEnvPlan,
} from "../../lib/test_runner/parallelRun";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("parallelRun: effectiveWorkerCount, serial fallback, and scale-up delay resolution", fn () {
    expectEq(effectiveWorkerCount(8, 3), @as(u32, 3));
    expectEq(effectiveWorkerCount(2, 10), @as(u32, 2));
    expect(!shouldUseParallelPool(4, 1));
    expect(!shouldUseParallelPool(1, 10));
    expect(shouldUseParallelPool(4, 2));

    expectEq(resolveScaleUpDelayMs(null, null), DEFAULT_PARALLEL_SCALE_UP_MS);
    expectEq(resolveScaleUpDelayMs(120, "50"), @as(i64, 120));
    expectEq(resolveScaleUpDelayMs(null, "75"), @as(i64, 75));
    expectEq(resolveScaleUpDelayMs(null, "-10"), @as(i64, 0));
    expectEq(resolveScaleUpDelayMs(null, "not-a-num"), DEFAULT_PARALLEL_SCALE_UP_MS);
});

test("parallelRun: lexicographic locality sorting and K-way contiguous range partitioning", fn () {
    const input: [5][]const u8 = [
        "z/last.test.cp",
        "a/02.test.cp",
        "m/mid.test.cp",
        "a/01.test.cp",
        "b/01.test.cp",
    ];
    const part = ParallelFilePartition.build(input[0..], 2, false) catch unreachable;
    expectEq(part.worker_count, @as(usize, 2));
    const sorted = part.fileSlice();
    expect(strEq(sorted[0], "a/01.test.cp"));
    expect(strEq(sorted[1], "a/02.test.cp"));
    expect(strEq(sorted[2], "b/01.test.cp"));
    expect(strEq(sorted[3], "m/mid.test.cp"));
    expect(strEq(sorted[4], "z/last.test.cp"));

    const r0 = part.rangeForWorker(0);
    const r1 = part.rangeForWorker(1);
    expectEq(r0.lo, @as(u32, 0));
    expectEq(r0.hi, @as(u32, 2));
    expectEq(r1.lo, @as(u32, 2));
    expectEq(r1.hi, @as(u32, 5));
    expectEq(r0.len() + r1.len(), @as(u32, 5));
});

test("parallelRun: --randomize preserves caller-shuffled order without re-sorting", fn () {
    const shuffled: [3][]const u8 = ["c.test.cp", "a.test.cp", "b.test.cp"];
    const part = ParallelFilePartition.build(shuffled[0..], 2, true) catch unreachable;
    const files = part.fileSlice();
    expect(strEq(files[0], "c.test.cp"));
    expect(strEq(files[1], "a.test.cp"));
    expect(strEq(files[2], "b.test.cp"));
});

test("parallelRun: WorkerFileDelta diff and 36-byte wire encode/decode roundtrip", fn () {
    const before = WorkerSummarySnapshot{
        pass: 10,
        fail: 1,
        skip: 2,
        todo: 1,
        expectations: 30,
        skipped_because_label: 0,
        files: 2,
        unhandled_errors: 0,
    };
    const after = WorkerSummarySnapshot{
        pass: 14,
        fail: 2,
        skip: 2,
        todo: 3,
        expectations: 42,
        skipped_because_label: 1,
        files: 3,
        unhandled_errors: 1,
    };
    const d = WorkerFileDelta.diff(7, before, after);
    expect(d.hasFailures());
    expectEq(d.pass, @as(u32, 4));
    expectEq(d.fail, @as(u32, 1));
    expectEq(d.todo, @as(u32, 2));
    expectEq(d.expectations, @as(u32, 12));
    expectEq(d.unhandled_errors, @as(u32, 1));

    let wire: [64]u8 = [0; 64];
    const n = d.encodePayload(wire[0..]) catch unreachable;
    expectEq(n, WORKER_FILE_DELTA_BYTES);

    const decoded = WorkerFileDelta.decodePayload(wire[0..n]) catch unreachable;
    expectEq(decoded.file_idx, @as(u32, 7));
    expectEq(decoded.pass, @as(u32, 4));
    expectEq(decoded.fail, @as(u32, 1));
    expectEq(decoded.expectations, @as(u32, 12));
    expectEq(decoded.skipped_because_label, @as(u32, 1));
    expectEq(decoded.files, @as(u32, 1));
    expectEq(decoded.unhandled_errors, @as(u32, 1));
});

test("parallelRun: WorkerArgvBuilder emits --test-worker --isolate and forwarded CLI options", fn () {
    const b = WorkerArgvBuilder.build(WorkerCliOptions{
        exe_path: "/usr/bin/cap",
        default_timeout_ms: 8000,
        max_concurrency: 12,
        dots: true,
        randomize: true,
        seed: 99,
        repeat_count: 3,
        retry: 2,
        filter_pattern: "auth",
    }) catch unreachable;

    expect(strEq(b.argAt(0), "/usr/bin/cap"));
    expect(strEq(b.argAt(1), "test"));
    expect(strEq(b.argAt(2), "--test-worker"));
    expect(strEq(b.argAt(3), "--isolate"));
    expect(strEq(b.argAt(4), "--timeout=8000"));
    expect(strEq(b.argAt(5), "--dots"));
    expect(strEq(b.argAt(6), "--randomize"));
    expect(strEq(b.argAt(7), "--seed=99"));
    expect(strEq(b.argAt(8), "--rerun-each=3"));
    expect(strEq(b.argAt(9), "--retry=2"));
    expect(strEq(b.argAt(10), "--max-concurrency=12"));
    expect(strEq(b.argAt(11), "-t"));
    expect(strEq(b.argAt(12), "auth"));
    expectEq(b.count(), @as(usize, 13));
});

test("parallelRun: WorkerEnvPlan 1-indexed IDs and truncated payload error guard", fn () {
    const env = WorkerEnvPlan.forWorker(3, true, true, false);
    expectEq(env.worker_id_1based, @as(u32, 4));
    expect(env.force_color);
    expect(env.needs_tmp_dir);
    expect(env.worker_junit_flag);

    let id_buf: [8]u8 = [0; 8];
    const id_len = env.formatWorkerId(id_buf[0..]) catch unreachable;
    expect(strEq(id_buf[0..id_len], "4"));

    let short_buf: [12]u8 = [0; 12];
    let failed_trunc = false;
    * = WorkerFileDelta.decodePayload(short_buf[0..]) catch {
        failed_trunc = true;
    };
    expect(failed_trunc);
});
