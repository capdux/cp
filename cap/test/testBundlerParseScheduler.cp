import { test, assert, assertEq } from "cap:test";
import {
    ParseSchedulerError,
    ScannedImportKind,
    ScannedImportEntry,
    scanImportSpecifiersFast,
    ParseTaskStatus,
    ParseFailureMode,
    ParseTaskEntry,
    ParseDispatchItem,
    ParseTaskScheduler,
    SourceLineTable,
    formatParseSchedulerReport,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("scanImportSpecifiersFast extracts static imports, re-exports, dynamic imports, and require calls", () => {
    const src =
        "import \"./polyfill.cp\";\n" ++
        "import { read, write } from \"cap:io\";\n" ++
        "import type { Config } from \"./types.cp\";\n" ++
        "export * from \"./utils.cp\";\n" ++
        "export { helper } from \"./helper.cp\";\n" ++
        "const mod = import(\"./lazy.cp\");\n" ++
        "const fs = require(\"node:fs\");\n";

    let out: [12]ScannedImportEntry = [ScannedImportEntry.empty(); 12];
    const sum = scanImportSpecifiersFast(src, out[0..]) catch unreachable;

    assertEq(sum.total_found, @as(usize, 7));
    assertEq(sum.static_imports, @as(u32, 3));
    assertEq(sum.static_reexports, @as(u32, 2));
    assertEq(sum.dynamic_imports, @as(u32, 1));
    assertEq(sum.cjs_requires, @as(u32, 1));

    assert(out[0].kind == ScannedImportKind.StaticImport && strEql(out[0].specifier, "./polyfill.cp"));
    assert(out[1].kind == ScannedImportKind.StaticImport && strEql(out[1].specifier, "cap:io"));
    assert(out[2].kind == ScannedImportKind.StaticImport && out[2].is_type_only && strEql(out[2].specifier, "./types.cp"));
    assert(out[3].kind == ScannedImportKind.StaticReExport && strEql(out[3].specifier, "./utils.cp"));
    assert(out[4].kind == ScannedImportKind.StaticReExport && strEql(out[4].specifier, "./helper.cp"));
    assert(out[5].kind == ScannedImportKind.DynamicImport && strEql(out[5].specifier, "./lazy.cp"));
    assert(out[6].kind == ScannedImportKind.CommonJsRequire && strEql(out[6].specifier, "node:fs"));
});

test("scanImportSpecifiersFast ignores fake imports inside comments, strings, templates, and member calls", () => {
    const src =
        "// import { fake1 } from \"comment-line\";\n" ++
        "/* require(\"comment-block\"); import(\"block-dyn\"); */\n" ++
        "const s1 = \"import { fake2 } from 'str-double'\";\n" ++
        "const s2 = `export * from \"template-str\"`;\n" ++
        "obj.require(\"member-not-cjs\");\n" ++
        "import { real } from \"./real.cp\";\n";

    let out: [8]ScannedImportEntry = [ScannedImportEntry.empty(); 8];
    const sum = scanImportSpecifiersFast(src, out[0..]) catch unreachable;

    assertEq(sum.total_found, @as(usize, 1));
    assert(sum.comments_skipped >= 2);
    assert(strEql(out[0].specifier, "./real.cp"));
});

test("ParseTaskScheduler deduplicates module IDs, boosts priority, honors max_workers, and fast-paths cache hits", () => {
    let tasks: [8]ParseTaskEntry = [ParseTaskEntry.empty(); 8];
    let sched = ParseTaskScheduler.init(tasks[0..], 2, ParseFailureMode.CollectAllErrors);

    const t0 = sched.enqueue(100, "src/low.cp", 1024, 5) catch unreachable;
    const t1 = sched.enqueue(200, "src/cached.cp", 2048, 10) catch unreachable;
    const t2 = sched.enqueue(300, "src/high.cp", 4096, 50) catch unreachable;
    // Duplicate module_id 100 with higher priority 100 -> deduplicated & boosted!
    const t0_dup = sched.enqueue(100, "src/low.cp", 1024, 100) catch unreachable;
    assertEq(t0_dup, t0);

    sched.markCachedHit(t1, 3) catch unreachable;

    let batch: [4]ParseDispatchItem = [ParseDispatchItem.empty(); 4];
    const n1 = sched.dispatchBatch(100, batch[0..]);
    assertEq(n1, @as(usize, 2));
    // t0 boosted to prio 100 dispatches before t2 (prio 50)
    assertEq(batch[0].task_index, t0);
    assertEq(batch[1].task_index, t2);
    // Worker pool full -> next dispatch yields 0
    assertEq(sched.dispatchBatch(110, batch[0..]), @as(usize, 0));

    sched.completeParsed(t0, 120, 4) catch unreachable;
    sched.completeParsed(t2, 130, 6) catch unreachable;
    assert(sched.isDrained());

    const st = sched.stats();
    assertEq(st.parsed, @as(u32, 2));
    assertEq(st.cached_hits, @as(u32, 1));
    assertEq(st.deduplicated_enqueues, @as(u32, 1));
    assertEq(st.total_imports_discovered, @as(u32, 13));
    assertEq(st.total_bytes_parsed, @as(u64, 5120));
});

test("ParseTaskScheduler enforces FailFast cancellation vs CollectAllErrors full diagnostic gathering", () => {
    // 1. FailFast cancels remaining queued tasks on first failure
    let tasks1: [4]ParseTaskEntry = [ParseTaskEntry.empty(); 4];
    let s1 = ParseTaskScheduler.init(tasks1[0..], 1, ParseFailureMode.FailFast);
    const a1 = s1.enqueue(1, "src/bad.cp", 512, 10) catch unreachable;
    const b1 = s1.enqueue(2, "src/queued.cp", 512, 1) catch unreachable;

    let batch: [2]ParseDispatchItem = [ParseDispatchItem.empty(); 2];
    assertEq(s1.dispatchBatch(0, batch[0..]), @as(usize, 1));
    s1.completeFailed(a1, 10, "E0100", 12, "unexpected token") catch unreachable;

    assert(s1.isDrained());
    assert(tasks1[b1].status == ParseTaskStatus.Cancelled);
    assertEq(s1.stats().cancelled, @as(u32, 1));

    // 2. CollectAllErrors continues parsing remaining tasks
    let tasks2: [4]ParseTaskEntry = [ParseTaskEntry.empty(); 4];
    let s2 = ParseTaskScheduler.init(tasks2[0..], 1, ParseFailureMode.CollectAllErrors);
    const a2 = s2.enqueue(1, "src/bad1.cp", 512, 10) catch unreachable;
    const b2 = s2.enqueue(2, "src/bad2.cp", 512, 1) catch unreachable;

    assertEq(s2.dispatchBatch(0, batch[0..]), @as(usize, 1));
    s2.completeFailed(a2, 10, "E0100", 5, "err 1") catch unreachable;
    assert(!s2.isDrained());
    assertEq(s2.dispatchBatch(10, batch[0..]), @as(usize, 1));
    assertEq(batch[0].task_index, b2);
    s2.completeFailed(b2, 20, "E0201", 8, "err 2") catch unreachable;
    assert(s2.isDrained());
    assertEq(s2.stats().failed, @as(u32, 2));
});

test("ParseTaskScheduler guards invalid task indices and illegal state transitions", () => {
    let tasks: [2]ParseTaskEntry = [ParseTaskEntry.empty(); 2];
    let sched = ParseTaskScheduler.init(tasks[0..], 2, ParseFailureMode.CollectAllErrors);
    const t0 = sched.enqueue(1, "src/a.cp", 100, 0) catch unreachable;

    // Completing a Queued (not Parsing) task fails with InvalidStateTransition
    let st_err = false;
    sched.completeParsed(t0, 10, 0) catch |e| {
        st_err = (e == ParseSchedulerError.InvalidStateTransition);
    };
    assert(st_err);

    // Out-of-bounds index fails with InvalidTaskIndex
    let idx_err = false;
    sched.markCachedHit(99, 0) catch |e| {
        idx_err = (e == ParseSchedulerError.InvalidTaskIndex);
    };
    assert(idx_err);
});

test("SourceLineTable resolves byte offsets in O(log L) via SIMD line index and renders AI-friendly diagnostics", () => {
    const src =
        "const a = 1;\r\n" ++
        "let x = unknownSym + 2;\r\n" ++
        "return x;\n";

    let starts: [8]usize = [0; 8];
    const ltab = SourceLineTable.build(src, starts[0..]) catch unreachable;
    assertEq(ltab.line_count, @as(usize, 3));

    // Line 2 starts at byte 14 ("const a = 1;\r\n" is 14 bytes); "unknownSym" starts at col 9 -> offset 14 + 8 = 22
    const loc = ltab.resolveOffset(src, 22);
    assertEq(loc.line, @as(u32, 2));
    assertEq(loc.column, @as(u32, 9));
    assert(strEql(src[loc.line_start_offset..loc.line_end_offset], "let x = unknownSym + 2;"));

    let dbuf: [512]u8 = [0; 512];
    const dlen = ltab.formatDiagnosticSnippet(
        src,
        "src/app.cp",
        "E0204",
        "undefined identifier 'unknownSym'",
        "declare 'unknownSym' or import it from a module",
        22,
        10,
        dbuf[0..],
    ) catch unreachable;
    const diag = dbuf[0..dlen];
    assert(strContains(diag, "error[E0204]: undefined identifier 'unknownSym'"));
    assert(strContains(diag, "--> src/app.cp:2:9"));
    assert(strContains(diag, "2 | let x = unknownSym + 2;"));
    assert(strContains(diag, "        ^^^^^^^^^^"));
    assert(strContains(diag, "= help: declare 'unknownSym' or import it from a module"));
});

test("formatParseSchedulerReport summarizes parsed/cached/failed counts, imports, and throughput", () => {
    let tasks: [4]ParseTaskEntry = [ParseTaskEntry.empty(); 4];
    let sched = ParseTaskScheduler.init(tasks[0..], 2, ParseFailureMode.CollectAllErrors);
    const t0 = sched.enqueue(1, "a.cp", 102400, 1) catch unreachable;
    const t1 = sched.enqueue(2, "b.cp", 51200, 1) catch unreachable;
    * = sched.enqueue(1, "a.cp", 102400, 1) catch unreachable; // deduped
    sched.markCachedHit(t1, 5) catch unreachable;

    let batch: [2]ParseDispatchItem = [ParseDispatchItem.empty(); 2];
    assertEq(sched.dispatchBatch(0, batch[0..]), @as(usize, 1));
    sched.completeParsed(t0, 100, 12) catch unreachable;

    let rbuf: [256]u8 = [0; 256];
    const rlen = formatParseSchedulerReport(sched.stats(), rbuf[0..]) catch unreachable;
    const report = rbuf[0..rlen];
    assert(strContains(report, "Parse Summary: 1 parsed, 1 cached, 0 failed (17 imports, 1 deduped) | 1000 KB/s"));
});
