// Source: /data/dev/bun/src/aio/ & /data/dev/bun/src/crash_handler/ & /data/dev/bun/src/test_runner/diff_format.zig
// ============================================================================
// cap/test/test_aio_safety_diff_format_deep.cp
// Comprehensive Deep Test Suite for cap:aio, cap:safety (CpuFeatures/OomGuard/CrashReport),
// and cap:patch (DiffFormatter / DiffFormatOptions)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ParentDeathWatchdog,
    AioOpKind,
    AioState,
    AioOp,
    AioCompletion,
    AioQueue,
    AIO_O_RDONLY,
    AIO_O_RDWR,
    AIO_O_CREAT,
    AIO_O_TRUNC,
} from "cap:aio";
import {
    CpuFeatures,
    OomPolicy,
    OomGuard,
    CrashReport,
} from "cap:safety";
import {
    DiffFormatOptions,
    DiffFormatter,
} from "cap:patch";

let g_oom_hook_hits: usize = 0;
let g_oom_hook_bytes: usize = 0;

fn onOomHook(requested_bytes: usize, context: []const u8) {
    * = context;
    g_oom_hook_hits += 1;
    g_oom_hook_bytes = requested_bytes;
}

fn testAioWatchdogDeep(): ! {
    let wd = ParentDeathWatchdog.init();
    const ppid = wd.parentPid();
    expect_eq(ppid > 0, true);
    expect_eq(wd.currentParentPid(), ppid);

    const armed_ok = try wd.install(15); // SIGTERM
    if (ppid > 1) {
        expect_eq(armed_ok, true);
        expect_eq(wd.isArmed(), true);
        expect_eq(wd.isParentAlive(), true);
    }

    if (comptime @import("builtin").os.tag == .linux) {
        const self_pid: i32 = @intCast(std.c.getpid());
        const proc_ppid = ParentDeathWatchdog.parentPidOf(self_pid);
        expect_eq(proc_ppid != null, true);
        expect_eq(proc_ppid.?, ppid);
    }

    try wd.clear();
    expect_eq(wd.isArmed(), false);
}

fn testAioQueueDeep(): ! {
    * = std.c.mkdir("temp", @as(std.c.mode_t, 0o755));

    let q = AioQueue<16>.init();
    expect_eq(q.capacity(), 16 as usize);
    expect_eq(q.isEmpty(), true);
    expect_eq(q.isFull(), false);

    // 1. Mkdir + Open
    const dir_path = "temp/aio_deep_test";
    const file_path = "temp/aio_deep_test/io_sample.bin";

    let comps: [16]AioCompletion = undefined;

    * = try q.submit(AioOp.mkdir(dir_path, 0o755, 10));
    const open_id = try q.submit(AioOp.open(file_path, AIO_O_RDWR | AIO_O_CREAT | AIO_O_TRUNC, 0o644, 20));
    expect_eq(q.pendingCount(), 2 as usize);

    const n1 = q.drain(&comps);
    expect_eq(n1, 2 as usize);
    expect_eq(comps[0].kind == AioOpKind.Mkdir, true);
    // Mkdir may return EEXIST (17) if directory already exists from prior run; both Ok and EEXIST are valid.
    expect_eq(comps[0].isOk() || comps[0].errno_code == 17, true);
    expect_eq(comps[1].id, open_id);
    expect_eq(comps[1].isOk(), true);
    expect_eq(comps[1].user_data, 20 as usize);
    const fd = comps[1].result_fd;
    expect_eq(fd >= 0, true);

    // 2. Batch submit: Write + Pwrite + Fsync + Cancelled Nop
    const part1 = "hello-aio-";
    const part2 = "ring!";
    const cancel_target = AioOp.nop(99);
    const batch: [4]AioOp = [
        AioOp.write(fd, part1, 30),
        AioOp.pwrite(fd, part2, 10, 31),
        AioOp.fsync(fd, 32),
        cancel_target,
    ];
    const submitted_n = try q.submitBatch(&batch);
    expect_eq(submitted_n, 4 as usize);

    // Cancel the last submitted op (its auto-assigned id is open_id + 4)
    const nop_id = open_id + 4;
    expect_eq(q.cancel(nop_id), true);
    expect_eq(q.cancel(999999), false);

    const n2 = q.drain(&comps);
    expect_eq(n2, 4 as usize);
    expect_eq(comps[0].isOk(), true);
    expect_eq(comps[0].bytes_transferred, 10 as usize);
    expect_eq(comps[1].isOk(), true);
    expect_eq(comps[1].bytes_transferred, 5 as usize);
    expect_eq(comps[2].isOk(), true);
    expect_eq(comps[3].isCancelled(), true);
    expect_eq(comps[3].errno_code, 125); // ECANCELED

    // 3. Pread + Stat + Close
    let read_buf: [32]u8 = [0; 32];
    * = try q.submit(AioOp.pread(fd, &read_buf, 0, 40));
    * = try q.submit(AioOp.stat(file_path, 41));
    * = try q.submit(AioOp.stat(dir_path, 42));
    * = try q.submit(AioOp.close(fd, 43));

    const n3 = q.drain(&comps);
    expect_eq(n3, 4 as usize);
    expect_eq(comps[0].isOk(), true);
    expect_eq(comps[0].bytes_transferred, 15 as usize);
    expect_eq(std.mem.eql(u8, read_buf[0..15], "hello-aio-ring!"), true);
    expect_eq(comps[1].isOk(), true);
    expect_eq(comps[1].stat_size, 15 as u64);
    expect_eq(comps[1].stat_is_dir, false);
    expect_eq(comps[2].isOk(), true);
    expect_eq(comps[2].stat_is_dir, true);
    expect_eq(comps[3].isOk(), true);

    // 4. Open read-only + Read + Close + Unlink + Stat missing
    * = try q.submit(AioOp.open(file_path, AIO_O_RDONLY, 0, 50));
    expect_eq(q.drain(&comps), 1 as usize);
    const ro_fd = comps[0].result_fd;
    expect_eq(ro_fd >= 0, true);

    let ro_buf: [32]u8 = [0; 32];
    * = try q.submit(AioOp.read(ro_fd, &ro_buf, 51));
    * = try q.submit(AioOp.close(ro_fd, 52));
    * = try q.submit(AioOp.unlink(file_path, 53));
    * = try q.submit(AioOp.stat(file_path, 54));

    const n4 = q.drain(&comps);
    expect_eq(n4, 4 as usize);
    expect_eq(comps[0].isOk(), true);
    expect_eq(std.mem.eql(u8, ro_buf[0..comps[0].bytes_transferred], "hello-aio-ring!"), true);
    expect_eq(comps[1].isOk(), true);
    expect_eq(comps[2].isOk(), true);
    expect_eq(comps[3].isFailed(), true);
    expect_eq(comps[3].errno_code, 2); // ENOENT

    // 5. Capacity enforcement on tiny queue
    let tiny = AioQueue<2>.init();
    * = try tiny.submit(AioOp.nop(1));
    * = try tiny.submit(AioOp.nop(2));
    expect_eq(tiny.isFull(), true);
    let overflowed = false;
    * = tiny.submit(AioOp.nop(3)) catch { overflowed = true; };
    expect_eq(overflowed, true);
}

fn testSafetyCpuAndCrashDeep(): ! {
    // 1. CpuFeatures detection & summary
    const cpu = CpuFeatures.detect();
    expect_eq(std.mem.eql(u8, cpu.arch_name, @tagName(@import("builtin").cpu.arch)), true);
    if (comptime @import("builtin").cpu.arch == .x86_64) {
        expect_eq(cpu.sse2, true);
        expect_eq(cpu.hasSimd128(), true);
    } else if (comptime @import("builtin").cpu.arch == .aarch64) {
        expect_eq(cpu.neon, true);
        expect_eq(cpu.hasSimd128(), true);
    }
    let summary_buf: [256]u8 = undefined;
    const summary = cpu.formatSummary(&summary_buf);
    expect_eq(summary.len > 0, true);
    expect_eq(std.mem.startsWith(u8, summary, cpu.arch_name), true);

    // 2. OomGuard policy, hook & allocation cap
    const alloc = std.heap.page_allocator;
    g_oom_hook_hits = 0;
    g_oom_hook_bytes = 0;

    let guard = OomGuard.init(OomPolicy.SoftFail);
    guard.setMaxSingleAlloc(64);
    guard.setHook(onOomHook);

    const ok_buf = guard.checkAlloc(alloc, 32, "small_buf");
    expect_eq(ok_buf != null, true);
    alloc.free(ok_buf.?);
    expect_eq(guard.count(), 0 as u64);

    const rejected = guard.checkAlloc(alloc, 128, "oversize_buf");
    expect_eq(rejected == null, true);
    expect_eq(guard.count(), 1 as u64);
    expect_eq(guard.lastRequestedBytes(), 128 as usize);
    expect_eq(g_oom_hook_hits, 1 as usize);
    expect_eq(g_oom_hook_bytes, 128 as usize);

    guard.recordOom(256, "manual_record");
    expect_eq(guard.count(), 2 as u64);

    // 3. CrashReport capture & Bun-style formatting
    expect_eq(std.mem.eql(u8, CrashReport.signalName(11), "SIGSEGV"), true);
    expect_eq(std.mem.eql(u8, CrashReport.signalName(6), "SIGABRT"), true);
    expect_eq(std.mem.eql(u8, CrashReport.signalName(8), "SIGFPE"), true);
    expect_eq(std.mem.eql(u8, CrashReport.signalName(4), "SIGILL"), true);

    const rep = CrashReport.capture(11, "null pointer dereference", "parsing config.cp", 0xDEADBEEF, guard.count());
    expect_eq(rep.pid > 0, true);
    expect_eq(std.mem.eql(u8, rep.signal_name, "SIGSEGV"), true);

    let report_buf: [2048]u8 = undefined;
    const report_txt = rep.formatReport(&report_buf);
    expect_eq(std.mem.indexOf(u8, report_txt, "CP Runtime Crash Report") != null, true);
    expect_eq(std.mem.indexOf(u8, report_txt, "SIGSEGV (11)") != null, true);
    expect_eq(std.mem.indexOf(u8, report_txt, "0xdeadbeef") != null, true);
    expect_eq(std.mem.indexOf(u8, report_txt, "null pointer dereference") != null, true);
    expect_eq(std.mem.indexOf(u8, report_txt, "parsing config.cp") != null, true);
    expect_eq(std.mem.indexOf(u8, report_txt, "OOM Count: 2") != null, true);

    const alloc_rep = try rep.formatReportAlloc(alloc);
    defer alloc.free(alloc_rep);
    expect_eq(std.mem.eql(u8, alloc_rep, report_txt), true);

    guard.reset();
    expect_eq(guard.count(), 0 as u64);
}

fn testPatchDiffFormatDeep(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Equal strings -> empty diff
    const eq_out = try DiffFormatter.format(alloc, "alpha\nbeta\n", "alpha\nbeta\n", DiffFormatOptions{});
    defer alloc.free(eq_out);
    expect_eq(eq_out.len, 0 as usize);

    // 2. Single-line colored diff with ANSI codes & inline highlight
    const colored_single = try DiffFormatter.format(
        alloc,
        "hello world!",
        "hello cp-world!",
        DiffFormatOptions{ use_color: true, show_line_numbers: true },
    );
    defer alloc.free(colored_single);
    expect_eq(std.mem.indexOf(u8, colored_single, "\x1b[32m+ Expected\x1b[0m") != null, true);
    expect_eq(std.mem.indexOf(u8, colored_single, "\x1b[31m- Received\x1b[0m") != null, true);
    expect_eq(std.mem.indexOf(u8, colored_single, "\x1b[7m") != null, true);

    // 3. Single-line uncolored diff -> no ESC codes, inline «...» highlight
    const plain_single = try DiffFormatter.format(
        alloc,
        "port: 8080;",
        "port: 9090;",
        DiffFormatOptions{ use_color: false, show_line_numbers: false, expected_label: "Want", received_label: "Got" },
    );
    defer alloc.free(plain_single);
    expect_eq(std.mem.indexOf(u8, plain_single, "\x1b[") == null, true);
    expect_eq(std.mem.indexOf(u8, plain_single, "+ Want") != null, true);
    expect_eq(std.mem.indexOf(u8, plain_single, "- Got") != null, true);
    expect_eq(std.mem.indexOf(u8, plain_single, "«808»0;") != null, true);
    expect_eq(std.mem.indexOf(u8, plain_single, "«909»0;") != null, true);

    // 4. Multi-line diff with context trimming ("... N unchanged lines ..."), gutter, and inline diff
    const exp_multi =
        "line 1\nline 2\nline 3\nline 4\nline 5\n" ++
        "const timeout = 5000;\n" ++
        "line 7\nline 8\nline 9\nline 10\nline 11\nline 12\n";
    const rec_multi =
        "line 1\nline 2\nline 3\nline 4\nline 5\n" ++
        "const timeout = 3000;\n" ++
        "line 7\nline 8\nline 9\nline 10\nline 11\nline 12\n";

    const fmt = DiffFormatter.init(DiffFormatOptions{
        use_color: false,
        context_lines: 1,
        show_line_numbers: true,
    });
    const multi_out = try fmt.render(alloc, exp_multi, rec_multi);
    defer alloc.free(multi_out);

    expect_eq(std.mem.indexOf(u8, multi_out, "... 4 unchanged lines ...") != null, true);
    expect_eq(std.mem.indexOf(u8, multi_out, "... 5 unchanged lines ...") != null, true);
    expect_eq(std.mem.indexOf(u8, multi_out, " 6 | + const timeout = «5»000;") != null, true);
    expect_eq(std.mem.indexOf(u8, multi_out, " 6 | - const timeout = «3»000;") != null, true);
    expect_eq(std.mem.indexOf(u8, multi_out, " 5 |   line 5") != null, true);
    expect_eq(std.mem.indexOf(u8, multi_out, " 7 |   line 7") != null, true);
}

fn main() {
    test("1. cap:aio deep — ParentDeathWatchdog (PR_SET_PDEATHSIG / kill 0 / proc stat) & AioQueue ring I/O", () => {
        try testAioWatchdogDeep();
        try testAioQueueDeep();
    });

    test("2. cap:safety deep — CpuFeatures CPUID detection, OomGuard policy/hook & CrashReport formatter", () => {
        try testSafetyCpuAndCrashDeep();
    });

    test("3. cap:patch deep — DiffFormatter bun-test colored diff, line numbers, context collapse & inline highlight", () => {
        try testPatchDiffFormatDeep();
    });
}
