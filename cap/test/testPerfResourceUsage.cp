// Production deep regression tests for lib/perf/resourceUsage.cp (cap:perf)

import { test, expect_eq } from "cap:test";
import {
    RUSAGE_USEC_PER_SEC,
    RUSAGE_BYTES_PER_KB,
    ResourceUsageError,
    CpuTimeUs,
    ResourceUsage,
} from "cap:perf";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("resourceUsage: constants, CPU time normalization & sub-metric views", () => {
    expect_eq(RUSAGE_USEC_PER_SEC, 1_000_000);
    expect_eq(RUSAGE_BYTES_PER_KB, 1_024);
    expect_eq(ResourceUsage.linuxMaxRssKbToBytes(2048), 2_097_152);

    const u = ResourceUsage{
        utime_sec: 1,
        utime_usec: 500_000,
        stime_sec: 0,
        stime_usec: 500_000,
        max_rss_bytes: 1_048_576,
        shared_memory_size: 65_536,
        swap_count: 1,
        in_block_ops: 12,
        out_block_ops: 8,
        ipc_sent: 5,
        ipc_received: 7,
        signal_count: 2,
        voluntary_ctx_switches: 30,
        involuntary_ctx_switches: 10,
    };

    const cpu = u.cpuTime();
    expect_eq(cpu.user_us, 1_500_000);
    expect_eq(cpu.system_us, 500_000);
    expect_eq(cpu.total_us, 2_000_000);
    expect_eq(cpu.userSharePermille(), 750);

    const ops = u.ops();
    expect_eq(ops.in_blocks, 12);
    expect_eq(ops.out_blocks, 8);
    expect_eq(ops.total(), 20);

    const msgs = u.messages();
    expect_eq(msgs.sent, 5);
    expect_eq(msgs.received, 7);
    expect_eq(msgs.total(), 12);

    const csw = u.contextSwitches();
    expect_eq(csw.voluntary, 30);
    expect_eq(csw.involuntary, 10);
    expect_eq(csw.total(), 40);
    expect_eq(csw.involuntaryRatioPermille(), 250);
});

test("resourceUsage: zero-safe ratios on empty snapshot", () => {
    const empty = ResourceUsage{};
    expect_eq(empty.cpuTime().total_us, 0);
    expect_eq(empty.cpuTime().userSharePermille(), 0);
    expect_eq(empty.contextSwitches().involuntaryRatioPermille(), 0);
    expect_eq(empty.ops().total(), 0);
    expect_eq(empty.messages().total(), 0);
});

test("resourceUsage: saturating diff between two snapshots", () => {
    const t0 = ResourceUsage{
        utime_sec: 1,
        utime_usec: 800_000,
        stime_sec: 0,
        stime_usec: 400_000,
        max_rss_bytes: 5_000_000,
        in_block_ops: 10,
        out_block_ops: 5,
        voluntary_ctx_switches: 20,
        involuntary_ctx_switches: 4,
    };
    const t1 = ResourceUsage{
        utime_sec: 2,
        utime_usec: 300_000,
        stime_sec: 0,
        stime_usec: 200_000, // deliberately smaller to test saturating sub
        max_rss_bytes: 4_000_000,
        in_block_ops: 18,
        out_block_ops: 3, // deliberately smaller
        voluntary_ctx_switches: 35,
        involuntary_ctx_switches: 9,
    };

    const d = t1.diff(t0);
    expect_eq(d.cpuTime().user_us, 500_000);
    expect_eq(d.cpuTime().system_us, 0);
    expect_eq(d.max_rss_bytes, 5_000_000);
    expect_eq(d.in_block_ops, 8);
    expect_eq(d.out_block_ops, 0);
    expect_eq(d.voluntary_ctx_switches, 15);
    expect_eq(d.involuntary_ctx_switches, 5);
});

test("resourceUsage: merge aggregates multiple worker snapshots", () => {
    const w1 = ResourceUsage{
        utime_sec: 0,
        utime_usec: 700_000,
        stime_sec: 0,
        stime_usec: 250_000,
        max_rss_bytes: 8_000_000,
        in_block_ops: 3,
        out_block_ops: 1,
        ipc_sent: 2,
        ipc_received: 4,
        voluntary_ctx_switches: 10,
        involuntary_ctx_switches: 2,
    };
    const w2 = ResourceUsage{
        utime_sec: 0,
        utime_usec: 600_000,
        stime_sec: 0,
        stime_usec: 150_000,
        max_rss_bytes: 12_000_000,
        in_block_ops: 7,
        out_block_ops: 9,
        ipc_sent: 5,
        ipc_received: 6,
        voluntary_ctx_switches: 25,
        involuntary_ctx_switches: 3,
    };

    const merged = w1.merge(w2);
    expect_eq(merged.utime_sec, 1);
    expect_eq(merged.utime_usec, 300_000);
    expect_eq(merged.stime_sec, 0);
    expect_eq(merged.stime_usec, 400_000);
    expect_eq(merged.cpuTime().total_us, 1_700_000);
    expect_eq(merged.max_rss_bytes, 12_000_000);
    expect_eq(merged.ops().total(), 20);
    expect_eq(merged.messages().total(), 17);
    expect_eq(merged.contextSwitches().total(), 40);
});

test("resourceUsage: formatSummary output & BufferTooSmall guard", () => {
    const u = ResourceUsage{
        utime_sec: 0,
        utime_usec: 1000,
        stime_sec: 0,
        stime_usec: 500,
        max_rss_bytes: 1048576,
        in_block_ops: 4,
        out_block_ops: 2,
        voluntary_ctx_switches: 12,
        involuntary_ctx_switches: 3,
    };
    let buf: [128]u8 = [0; 128];
    const n = u.formatSummary(buf[0..]) catch 0;
    expect_eq(strEq(buf[0..n], "rss=1048576B cpu=1500us(u=1000,s=500) io=4/2 csw=12/3"), true);

    let tiny: [16]u8 = [0; 16];
    let err_hit = false;
    * = u.formatSummary(tiny[0..]) catch |e| {
        err_hit = (e == ResourceUsageError.BufferTooSmall);
        0
    };
    expect_eq(err_hit, true);

    // Direct CpuTimeUs helper check
    const ct = CpuTimeUs.fromTimevals(2, 100, 1, 200);
    expect_eq(ct.total_us, 3_000_300);
});
