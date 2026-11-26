import { test, expect_eq } from "cap:test";
import {
    RANGE_OK,
    RANGE_UNSATISFIABLE,
    RANGE_MALFORMED,
    parseRange,
    formatContentRange,
    formatUnsatisfiableRange,
} from "../../packages/@cap/range-parser/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/range-parser — Standard bytes=0-499 & end >= size auto-clamping", () => {
        const r1 = parseRange(1000, "bytes=0-499", false);
        expect_eq(r1.status, RANGE_OK);
        expect_eq(r1.count, 1 as usize);
        expect_eq(r1.ranges[0].start, 0 as u64);
        expect_eq(r1.ranges[0].end, 499 as u64);

        const r2 = parseRange(1000, "bytes=200-5000", false);
        expect_eq(r2.status, RANGE_OK);
        expect_eq(r2.count, 1 as usize);
        expect_eq(r2.ranges[0].start, 200 as u64);
        expect_eq(r2.ranges[0].end, 999 as u64);
    });

    test("2. @cap/range-parser — Open-ended range bytes=500-", () => {
        const r = parseRange(1000, "bytes=500-", false);
        expect_eq(r.status, RANGE_OK);
        expect_eq(r.count, 1 as usize);
        expect_eq(r.ranges[0].start, 500 as u64);
        expect_eq(r.ranges[0].end, 999 as u64);
    });

    test("3. @cap/range-parser — Suffix range bytes=-200 & oversized suffix clamping", () => {
        const r1 = parseRange(1000, "bytes=-200", false);
        expect_eq(r1.status, RANGE_OK);
        expect_eq(r1.count, 1 as usize);
        expect_eq(r1.ranges[0].start, 800 as u64);
        expect_eq(r1.ranges[0].end, 999 as u64);

        // Suffix larger than resource size clamps to [0, size-1]
        const r2 = parseRange(1000, "bytes=-5000", false);
        expect_eq(r2.status, RANGE_OK);
        expect_eq(r2.ranges[0].start, 0 as u64);
        expect_eq(r2.ranges[0].end, 999 as u64);
    });

    test("4. @cap/range-parser — Multi-range combine=false preserves order & combine=true merges", () => {
        const hdr = "bytes=150-250, 0-99, 100-180, 500-599";

        const raw = parseRange(1000, hdr, false);
        expect_eq(raw.status, RANGE_OK);
        expect_eq(raw.count, 4 as usize);
        expect_eq(raw.ranges[0].start, 150 as u64);
        expect_eq(raw.ranges[1].start, 0 as u64);

        // 0-99 + 100-180 + 150-250 merge into [0, 250]; 500-599 stays separate
        const merged = parseRange(1000, hdr, true);
        expect_eq(merged.status, RANGE_OK);
        expect_eq(merged.count, 2 as usize);
        expect_eq(merged.ranges[0].start, 0 as u64);
        expect_eq(merged.ranges[0].end, 250 as u64);
        expect_eq(merged.ranges[1].start, 500 as u64);
        expect_eq(merged.ranges[1].end, 599 as u64);
    });

    test("5. @cap/range-parser — HTTP 416 RANGE_UNSATISFIABLE & RANGE_MALFORMED detection", () => {
        // Out of bounds / empty resource / reversed range / -0
        expect_eq(parseRange(1000, "bytes=1000-2000", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(0, "bytes=0-10", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(1000, "bytes=500-100", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(1000, "bytes=-0", false).status, RANGE_UNSATISFIABLE);

        // Malformed syntax / wrong unit
        expect_eq(parseRange(1000, "items=0-10", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "bytes=", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "bytes=-", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "bytes=abc-def", false).status, RANGE_MALFORMED);
    });

    test("6. @cap/range-parser — formatContentRange & formatUnsatisfiableRange output", () => {
        let buf: [64]u8 = undefined;
        const n1 = formatContentRange(0, 499, 1000, buf[0..64]);
        expect_eq(n1, 16 as i64);
        expect_eq(sliceEq(buf[0..16], "bytes 0-499/1000"), true);

        const n2 = formatUnsatisfiableRange(12345, buf[0..64]);
        expect_eq(n2, 13 as i64);
        expect_eq(sliceEq(buf[0..13], "bytes */12345"), true);

        // Undersized destination buffer returns -1
        let tiny: [5]u8 = undefined;
        expect_eq(formatContentRange(0, 499, 1000, tiny[0..5]), -1 as i64);
        expect_eq(formatUnsatisfiableRange(12345, tiny[0..5]), -1 as i64);
    });
}
