import { test, expect_eq } from "cap:test";
import {
    ProgressState,
    percent,
    etaMs,
    bytesPerSec,
    renderBar,
    renderLine,
} from "../../packages/@cap/indicatif/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsSub(hay: []const u8, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= hay.len) {
        if (strEq(hay[i .. i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn main() {
    test("1. @cap/indicatif — percent() 0%, 50%, 100%, >100% clamping & total=0", () => {
        expect_eq(percent(ProgressState { pos: 0, total: 100, elapsed_ms: 0, width: 10 }), 0.0);
        expect_eq(percent(ProgressState { pos: 50, total: 100, elapsed_ms: 1000, width: 10 }), 50.0);
        expect_eq(percent(ProgressState { pos: 100, total: 100, elapsed_ms: 2000, width: 10 }), 100.0);
        expect_eq(percent(ProgressState { pos: 250, total: 100, elapsed_ms: 2000, width: 10 }), 100.0);
        expect_eq(percent(ProgressState { pos: 10, total: 0, elapsed_ms: 100, width: 10 }), 0.0);
    });

    test("2. @cap/indicatif — etaMs() & bytesPerSec() throughput accuracy", () => {
        const s = ProgressState { pos: 500, total: 2000, elapsed_ms: 1000, width: 10 };
        expect_eq(etaMs(s), 3000);
        expect_eq(absF64(bytesPerSec(s) - 500.0) < 0.000001, true);

        expect_eq(etaMs(ProgressState { pos: 0, total: 1000, elapsed_ms: 500, width: 10 }), 0);
        expect_eq(etaMs(ProgressState { pos: 1000, total: 1000, elapsed_ms: 500, width: 10 }), 0);
        expect_eq(bytesPerSec(ProgressState { pos: 100, total: 1000, elapsed_ms: 0, width: 10 }), 0.0);
    });

    test("3. @cap/indicatif — renderBar() exact 0%, 50% & 100% layouts", () => {
        let buf: [32]u8 = undefined;

        const n0 = renderBar(ProgressState { pos: 0, total: 100, elapsed_ms: 0, width: 10 }, 61, 62, 32, &buf);
        expect_eq(strEq(buf[0..n0], "[          ]"), true);

        const n50 = renderBar(ProgressState { pos: 50, total: 100, elapsed_ms: 1000, width: 10 }, 61, 62, 32, &buf);
        expect_eq(strEq(buf[0..n50], "[====>     ]"), true);

        const n100 = renderBar(ProgressState { pos: 100, total: 100, elapsed_ms: 2000, width: 10 }, 61, 62, 32, &buf);
        expect_eq(strEq(buf[0..n100], "[==========]"), true);
    });

    test("4. @cap/indicatif — renderLine() composes @cap/pretty-bytes & @cap/pretty-ms", () => {
        const st = ProgressState { pos: 512, total: 1024, elapsed_ms: 2000, width: 10 };
        let buf: [128]u8 = undefined;
        const n = renderLine(st, "Fetch", &buf);
        const line: []const u8 = buf[0..n];

        expect_eq(containsSub(line, "Fetch [====>     ] 50% "), true);
        expect_eq(containsSub(line, "512 B/"), true);
        expect_eq(containsSub(line, " ETA 2s"), true);
    });

    test("5. @cap/indicatif — Wide progress bar & zero-width bar boundaries", () => {
        let buf: [64]u8 = undefined;
        const n_zero = renderBar(ProgressState { pos: 50, total: 100, elapsed_ms: 100, width: 0 }, 61, 62, 32, &buf);
        expect_eq(strEq(buf[0..n_zero], "[]"), true);

        const n_wide = renderBar(ProgressState { pos: 100, total: 100, elapsed_ms: 100, width: 40 }, 35, 35, 45, &buf);
        expect_eq(n_wide, 42);
        expect_eq(buf[0], 91);  // '['
        expect_eq(buf[1], 35);  // '#'
        expect_eq(buf[41], 93); // ']'
    });

    test("6. @cap/indicatif — Small buffer truncation safety", () => {
        const st = ProgressState { pos: 512, total: 1024, elapsed_ms: 2000, width: 10 };
        let small: [8]u8 = undefined;
        const n1 = renderBar(st, 61, 62, 32, &small);
        expect_eq(n1, 8);
        expect_eq(strEq(small[0..n1], "[====>  "), true);

        const n2 = renderLine(st, "Download", &small);
        expect_eq(n2, 8);
        expect_eq(strEq(small[0..n2], "Download"), true);

        const empty_out: []u8 = small[0..0];
        expect_eq(renderBar(st, 61, 62, 32, empty_out), 0);
        expect_eq(renderLine(st, "Download", empty_out), 0);
    });
}
