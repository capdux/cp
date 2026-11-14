// cap/test/test_cli_watch_patch_bench.cp
// E2E Regression Suite for deepened modules in cap:cli, cap:watch, cap:patch, and cap:bench

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CliTableAlign, CliTable } from "cap:cli";
import { WatchFilter, DebounceTracker } from "cap:watch";
import { DiffOpKind, DiffOp, LineDiffer } from "cap:patch";
import { LatencyHistogram } from "cap:bench";

fn main() {
    test("1. cap:cli — CliTable auto-width ASCII table formatting & alignment", () => {
        let tbl = CliTable.init(3);
        tbl.setHeader(0, "Module", CliTableAlign.Left);
        tbl.setHeader(1, "Status", CliTableAlign.Center);
        tbl.setHeader(2, "Tests", CliTableAlign.Right);

        expect_eq(tbl.addRow3("cap:cli", "PASS", "12"), true);
        expect_eq(tbl.addRow3("cap:watch", "PASS", "108"), true);

        let out_buf: [512]u8 = [0; 512];
        const written = tbl.renderToBuf(out_buf[0..512]);
        expect_eq(written > 0, true);
        const rendered = out_buf[0..written];
        expect_eq(std.mem.indexOf(u8, rendered, "| Module    | Status | Tests |") != null, true);
        expect_eq(std.mem.indexOf(u8, rendered, "| cap:cli   |  PASS  |    12 |") != null, true);
        expect_eq(std.mem.indexOf(u8, rendered, "| cap:watch |  PASS  |   108 |") != null, true);
    });

    test("2. cap:watch — WatchFilter path ignore rules & DebounceTracker<CAP>", () => {
        expect_eq(WatchFilter.shouldIgnore(".git/HEAD"), true);
        expect_eq(WatchFilter.shouldIgnore("packages/app/node_modules/pkg/index.js"), true);
        expect_eq(WatchFilter.shouldIgnore("build/.cap/zig/out.zig"), true);
        expect_eq(WatchFilter.shouldIgnore("src/main.cp.swp"), true);
        expect_eq(WatchFilter.shouldIgnore("src/main.cp~"), true);
        expect_eq(WatchFilter.shouldIgnore("src/main.cp"), false);
        expect_eq(WatchFilter.shouldIgnore("lib/watch/filter.cp"), false);

        // 50,000,000 ns (50ms) debounce window
        let debouncer = DebounceTracker<8>.init(50000000);
        expect_eq(debouncer.shouldEmit("src/main.cp", 100000000), true);
        // Duplicate within 20ms -> suppressed
        expect_eq(debouncer.shouldEmit("src/main.cp", 120000000), false);
        // Different file at 120ms -> emitted
        expect_eq(debouncer.shouldEmit("src/other.cp", 120000000), true);
        // Same file after 60ms -> emitted
        expect_eq(debouncer.shouldEmit("src/main.cp", 160000000), true);
    });

    test("3. cap:patch — LineDiffer common prefix/suffix & unified diff formatting", () => {
        const old_txt = "line1\nold_mid\nline3\n";
        const new_txt = "line1\nnew_mid_a\nnew_mid_b\nline3\n";

        let ops: [16]DiffOp = undefined;
        const op_count = LineDiffer.diffLines(old_txt, new_txt, ops[0..16]);
        expect_eq(op_count, 5 as usize);
        expect_eq(ops[0].kind == DiffOpKind.Equal, true);
        expect_eq(ops[1].kind == DiffOpKind.Delete, true);
        expect_eq(ops[2].kind == DiffOpKind.Insert, true);
        expect_eq(ops[3].kind == DiffOpKind.Insert, true);
        expect_eq(ops[4].kind == DiffOpKind.Equal, true);

        let diff_buf: [256]u8 = [0; 256];
        const formatted = LineDiffer.formatUnifiedToBuf(ops[0..op_count], diff_buf[0..256]);
        const expected = " line1\n-old_mid\n+new_mid_a\n+new_mid_b\n line3\n";
        expect_eq(std.mem.eql(u8, formatted, expected), true);
    });

    test("4. cap:bench — LatencyHistogram logarithmic buckets, mean/min/max & percentile bounds", () => {
        let hist = LatencyHistogram.init();
        hist.record(50);        // bucket 0 (< 100ns)
        hist.record(400);       // bucket 1 (< 1,000ns)
        hist.record(600);       // bucket 1 (< 1,000ns)
        hist.record(5000);      // bucket 2 (< 10,000ns)
        hist.record(250000);    // bucket 4 (< 1,000,000ns)

        expect_eq(hist.count(), 5 as u64);
        expect_eq(hist.bucketCount(0), 1 as u64);
        expect_eq(hist.bucketCount(1), 2 as u64);
        expect_eq(hist.bucketCount(2), 1 as u64);
        expect_eq(hist.bucketCount(4), 1 as u64);
        expect_eq(hist.minNs(), 50 as u64);
        expect_eq(hist.maxNs(), 250000 as u64);
        expect_eq(hist.meanNs(), 51210 as u64);
        expect_eq(hist.estimatePercentileBoundNs(50), 1000 as u64);
        expect_eq(hist.estimatePercentileBoundNs(99), 1000000 as u64);

        hist.reset();
        expect_eq(hist.count(), 0 as u64);
    });
}
