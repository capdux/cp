// Deep production test suite for lib/crash_handler (cap:crash_handler & cap.crash_handler)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    SignalKind,
    CrashReason,
    StackFrame,
    Breadcrumb,
    MetadataEntry,
    CrashReport,
    CrashReportBuilder,
    FixedCrashWriter,
    BreadcrumbRing,
    signalName,
    signalDescription,
    signalFromCode,
    reasonName,
    reasonSignal,
    reasonAddress,
    formatCrashTextFixed,
    formatCrashText,
    formatCrashJson,
} from "cap:crash_handler";

fn runFormatTest(): ! {
    let alloc = std.testing.allocator;
    let frames: [1]StackFrame = [
        StackFrame{
            .addr = 0x7fff0010,
            .symbol = "parse\"json\\node",
            .file = "src/parser.cp",
            .line = 88,
            .col = 3,
            .offset = 0x4,
        },
    ];
    let crumbs: [1]Breadcrumb = [
        Breadcrumb{
            .timestamp_ms = 999,
            .category = "io",
            .message = "line1\nline2\t\x01",
            .level = 3,
        },
    ];
    let meta: [1]MetadataEntry = [
        MetadataEntry{ .key = "version", .value = "1.0.0" },
    ];

    let report = CrashReport.init(
        CrashReason{ .Panic = "bad \"state\"\nnow" },
        0x7fff0010,
        42,
        1700000001000,
        frames[0..1],
        crumbs[0..1],
        meta[0..1],
    );

    let text = formatCrashText(&report, alloc)?;
    defer alloc.free(text);
    expect_eq(std.mem.indexOf(u8, text, "=== CRASH REPORT ===") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "PANIC: Runtime panic at 0x7fff0010") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "#0 0x7fff0010 in parse\"json\\node+0x4") != null, true);

    let json = formatCrashJson(&report, alloc)?;
    defer alloc.free(json);
    expect_eq(std.mem.indexOf(u8, json, "\"signal\":\"PANIC\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "bad \\\"state\\\"\\nnow") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "parse\\\"json\\\\node") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "line1\\nline2\\t\\u0001") != null, true);
}

fn runDynamicMountTest(): ! {
    let alloc = std.testing.allocator;

    let builder = cap.crash_handler.CrashReportBuilder.init(
        cap.crash_handler.CrashReason{ .BusError = 0xbeef },
        0xbeef,
        777,
        1700000002000,
    );
    defer builder.deinit(alloc);

    builder.addFrame(alloc, cap.crash_handler.StackFrame{
        .addr = 0x1234,
        .symbol = "handler",
        .file = "app.cp",
        .line = 5,
        .col = 1,
        .offset = 0,
    })?;
    builder.addBreadcrumb(alloc, cap.crash_handler.Breadcrumb{
        .timestamp_ms = 123,
        .category = "sys",
        .message = "started",
        .level = 1,
    })?;
    builder.addMetadata(alloc, "host", "node-1")?;

    let report = builder.build(alloc)?;
    defer report.deinit(alloc);

    let fw = cap.crash_handler.FixedCrashWriter(512).init();
    cap.crash_handler.formatCrashTextFixed(512, &report, &fw);
    expect_eq(fw.isTruncated(), false);
    expect_eq(std.mem.indexOf(u8, fw.slice(), "SIGBUS") != null, true);

    let json = cap.crash_handler.formatCrashJson(&report, alloc)?;
    defer alloc.free(json);
    expect_eq(std.mem.indexOf(u8, json, "\"signal\":\"SIGBUS\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"thread_id\":777") != null, true);
}

test("1. SignalKind table mapping and fault address description", () => {
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Segv), "SIGSEGV"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Abrt), "SIGABRT"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Bus), "SIGBUS"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Ill), "SIGILL"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Fpe), "SIGFPE"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Trap), "SIGTRAP"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Panic), "PANIC"), true);
    expect_eq(std.mem.eql(u8, signalName(SignalKind.Unknown), "UNKNOWN"), true);

    expect_eq(signalFromCode(11) == SignalKind.Segv, true);
    expect_eq(signalFromCode(6) == SignalKind.Abrt, true);
    expect_eq(signalFromCode(7) == SignalKind.Bus, true);
    expect_eq(signalFromCode(4) == SignalKind.Ill, true);
    expect_eq(signalFromCode(8) == SignalKind.Fpe, true);
    expect_eq(signalFromCode(5) == SignalKind.Trap, true);
    expect_eq(signalFromCode(999) == SignalKind.Unknown, true);

    expect_eq(std.mem.indexOf(u8, signalDescription(SignalKind.Segv), "Segmentation fault") != null, true);

    let reason = CrashReason{ .SegmentationFault = 0xdeadbeef };
    expect_eq(reasonSignal(reason) == SignalKind.Segv, true);
    expect_eq(std.mem.eql(u8, reasonName(reason), "segmentation_fault"), true);
    let addr = reasonAddress(reason);
    expect_eq(addr != null, true);
    if (a := addr) {
        expect_eq(a, 0xdeadbeef);
    }
    expect_eq(reasonAddress(CrashReason.OutOfMemory) == null, true);
});

test("2. FixedCrashWriter zero-heap truncation protection and redaction", () => {
    let small = FixedCrashWriter(16).init();
    expect_eq(small.isTruncated(), false);
    small.writeBytes("0123456789");
    expect_eq(small.len(), 10);
    expect_eq(small.isTruncated(), false);

    small.writeBytes("abcdefghij");
    expect_eq(small.len(), 16);
    expect_eq(small.isTruncated(), true);
    expect_eq(std.mem.eql(u8, small.slice(), "0123456789abcdef"), true);

    small.reset();
    expect_eq(small.len(), 0);
    expect_eq(small.isTruncated(), false);

    let w = FixedCrashWriter(512).init();
    w.writeSignalSummary(SignalKind.Segv, 0x1020);
    w.writeRedactedEnv("API_SECRET_KEY", "super_secret_123");
    w.writeRedactedEnv("APP_ENV", "production");
    w.writeFrame(0, StackFrame{
        .addr = 0x400500,
        .symbol = "main_entry",
        .file = "src/main.cp",
        .line = 12,
        .col = 4,
        .offset = 0x18,
    });
    expect_eq(w.isTruncated(), false);
    let out = w.slice();
    expect_eq(std.mem.indexOf(u8, out, "SIGSEGV") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "0x1020") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "API_SECRET_KEY=[REDACTED]") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "super_secret_123") == null, true);
    expect_eq(std.mem.indexOf(u8, out, "APP_ENV=production") != null, true);
    expect_eq(std.mem.indexOf(u8, out, "main_entry+0x18 (src/main.cp:12:4)") != null, true);
});

test("3. BreadcrumbRing wrap-around overwrite and chronological replay", () => {
    let ring = BreadcrumbRing(3).init();
    expect_eq(ring.len(), 0);
    expect_eq(ring.capacity(), 3);

    ring.push(Breadcrumb{ .timestamp_ms = 10, .category = "boot", .message = "init", .level = 1 });
    ring.push(Breadcrumb{ .timestamp_ms = 20, .category = "net", .message = "listen", .level = 1 });
    ring.push(Breadcrumb{ .timestamp_ms = 30, .category = "req", .message = "GET /", .level = 2 });
    ring.push(Breadcrumb{ .timestamp_ms = 40, .category = "db", .message = "query", .level = 2 });
    ring.push(Breadcrumb{ .timestamp_ms = 50, .category = "err", .message = "null ptr", .level = 4 });

    expect_eq(ring.len(), 3);
    expect_eq(ring.total_pushed, 5);

    let it = ring.iterFromOldest();
    let expected_ts: [3]u64 = [ 30, 40, 50 ];
    let idx: usize = 0;
    while (crumb := it.next()) {
        expect_eq(crumb.timestamp_ms, expected_ts[idx]);
        idx += 1;
    }
    expect_eq(idx, 3);

    ring.clear();
    expect_eq(ring.len(), 0);
    expect_eq(ring.getFromOldest(0) == null, true);
});

test("4. formatCrashText and formatCrashJson with RFC 8259 escaping", () => {
    runFormatTest() catch unreachable;
});

test("5. cap.crash_handler dynamic mount and zero-leak builder lifecycle", () => {
    runDynamicMountTest() catch unreachable;
});
