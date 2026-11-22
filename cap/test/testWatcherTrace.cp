// Deep production regression tests for cap:watcher trace.cp
// Covers: NDJSON line formatting matching Bun WatcherTrace.writeEvents,
// per-path deduplication & op-mask merging, changed-name deduplication & omission when empty,
// empty path fallback to "(unknown)", RFC 8259 JSON escaping (quotes/backslashes/control chars),
// negative/zero timestamp formatting, and buffer-too-small / capacity error guards.

import { test, expect, expectEq } from "cap:test";
import {
    WATCHER_TRACE_ENV_VAR,
    WatchTraceOpMask,
    WatchTraceEntry,
    WatchTraceBatch,
    formatJsonEscapedStr,
} from "cap:watcher";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("watcherTrace: formats NDJSON line with merged ops and deduplicated changed names", fn () {
    expect(strEq(WATCHER_TRACE_ENV_VAR, "BUN_WATCHER_TRACE"));

    let batch = WatchTraceBatch.init(1700000000123);
    const idx0 = batch.addOrMergePath("src", WatchTraceOpMask{ write: true, is_dir: true }, "app.cp") catch unreachable;
    const idx1 = batch.addOrMergePath("src", WatchTraceOpMask{ metadata: true }, "util.cp") catch unreachable;
    const idx2 = batch.addOrMergePath("src", WatchTraceOpMask{ write: true }, "app.cp") catch unreachable;
    expectEq(idx0, @as(usize, 0));
    expectEq(idx1, @as(usize, 0));
    expectEq(idx2, @as(usize, 0));
    expectEq(batch.entrySlice().len, @as(usize, 1));
    expectEq(batch.entrySlice()[0].changedSlice().len, @as(usize, 2));

    let buf: [512]u8 = [0; 512];
    const n = batch.formatNdjsonLine(buf[0..]) catch unreachable;
    expect(strEq(
        buf[0..n],
        "{\"timestamp\":1700000000123,\"files\":{\"src\":{\"events\":[\"metadata\",\"write\",\"is_dir\"],\"changed\":[\"app.cp\",\"util.cp\"]}}}\n",
    ));
});

test("watcherTrace: omits changed key when absent and uses (unknown) for empty path", fn () {
    let batch = WatchTraceBatch.init(0);
    * = batch.addOrMergePath("", WatchTraceOpMask{ delete: true, rename: true }, null) catch unreachable;

    let buf: [256]u8 = [0; 256];
    const n = batch.formatNdjsonLine(buf[0..]) catch unreachable;
    expect(strEq(
        buf[0..n],
        "{\"timestamp\":0,\"files\":{\"(unknown)\":{\"events\":[\"delete\",\"rename\"]}}}\n",
    ));
});

test("watcherTrace: escapes quotes, backslashes, newlines, tabs, and control bytes per RFC 8259", fn () {
    let out: [128]u8 = [0; 128];
    const raw: [9]u8 = ['a', '"', '\\', '\n', '\r', '\t', 0x08, 0x0c, 0x01];
    const n = formatJsonEscapedStr(raw[0..], out[0..]) catch unreachable;
    expect(strEq(out[0..n], "\"a\\\"\\\\\\n\\r\\t\\b\\f\\u0001\""));

    let batch = WatchTraceBatch.init(-42);
    * = batch.addOrMergePath("C:\\work\\my\"dir", WatchTraceOpMask{ create: true }, "f\t1.cp") catch unreachable;
    let line: [256]u8 = [0; 256];
    const ln = batch.formatNdjsonLine(line[0..]) catch unreachable;
    expect(strEq(
        line[0..ln],
        "{\"timestamp\":-42,\"files\":{\"C:\\\\work\\\\my\\\"dir\":{\"events\":[\"create\"],\"changed\":[\"f\\t1.cp\"]}}}\n",
    ));
});

test("watcherTrace: handles multiple files and all 8 WatchTraceOpMask flags", fn () {
    const empty_op = WatchTraceOpMask{};
    expect(!empty_op.any());
    const all_op = WatchTraceOpMask{
        delete: true,
        metadata: true,
        rename: true,
        write: true,
        move_to: true,
        create: true,
        access: true,
        is_dir: true,
    };
    expect(all_op.any());

    let batch = WatchTraceBatch.init(99);
    * = batch.addEntry(WatchTraceEntry{ file_path: "a.cp", op: all_op }) catch unreachable;
    * = batch.addEntry(WatchTraceEntry{ file_path: "b.cp", op: WatchTraceOpMask{ move_to: true, access: true } }) catch unreachable;

    let buf: [512]u8 = [0; 512];
    const n = batch.formatNdjsonLine(buf[0..]) catch unreachable;
    expect(strEq(
        buf[0..n],
        "{\"timestamp\":99,\"files\":{\"a.cp\":{\"events\":[\"delete\",\"metadata\",\"rename\",\"write\",\"move_to\",\"create\",\"access\",\"is_dir\"]},\"b.cp\":{\"events\":[\"move_to\",\"access\"]}}}\n",
    ));
});

test("watcherTrace: guards against tiny output buffer and batch reset", fn () {
    let batch = WatchTraceBatch.init(100);
    * = batch.addOrMergePath("src/main.cp", WatchTraceOpMask{ write: true }, null) catch unreachable;

    let tiny: [16]u8 = [0; 16];
    let err_small = false;
    * = batch.formatNdjsonLine(tiny[0..]) catch {
        err_small = true;
    };
    expect(err_small);

    batch.reset(200);
    expectEq(batch.entrySlice().len, @as(usize, 0));
    let buf: [64]u8 = [0; 64];
    const n = batch.formatNdjsonLine(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n], "{\"timestamp\":200,\"files\":{}}\n"));
});
