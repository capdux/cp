import { test, expect_eq } from "cap:test";
import {
    parseRange, rangeByteLength, RANGE_OK, RANGE_UNSATISFIABLE, RANGE_MALFORMED
} from "../../packages/range-parser/src/index.cp";

fn main() {
    test("1. @cap/range-parser — Standard explicit, open-ended & suffix ranges", () => {
        let r1 = parseRange(1000, "bytes=0-499", false);
        expect_eq(r1.status, RANGE_OK);
        expect_eq(r1.unit, "bytes");
        expect_eq(r1.count, 1);
        expect_eq(r1.ranges[0].start, 0);
        expect_eq(r1.ranges[0].end, 499);
        expect_eq(rangeByteLength(r1.ranges[0]), 500);

        let r2 = parseRange(1000, "bytes=500-", false);
        expect_eq(r2.ranges[0].start, 500);
        expect_eq(r2.ranges[0].end, 999);

        let r3 = parseRange(1000, "bytes=-200", false);
        expect_eq(r3.ranges[0].start, 800);
        expect_eq(r3.ranges[0].end, 999);
    });

    test("2. @cap/range-parser — Clamps end beyond resource size", () => {
        let r = parseRange(1000, "bytes=800-5000", false);
        expect_eq(r.status, RANGE_OK);
        expect_eq(r.ranges[0].start, 800);
        expect_eq(r.ranges[0].end, 999);

        let rs = parseRange(100, "bytes=-500", false);
        expect_eq(rs.ranges[0].start, 0);
        expect_eq(rs.ranges[0].end, 99);
    });

    test("3. @cap/range-parser — Multiple ranges without combining", () => {
        let r = parseRange(1000, "bytes=0-99, 200-299, 500-599", false);
        expect_eq(r.status, RANGE_OK);
        expect_eq(r.count, 3);
        expect_eq(r.ranges[1].start, 200);
        expect_eq(r.ranges[2].end, 599);
    });

    test("4. @cap/range-parser — combine=true sorts and merges overlapping/adjacent ranges", () => {
        let r = parseRange(1000, "bytes=200-300, 0-100, 101-250, 800-900", true);
        expect_eq(r.status, RANGE_OK);
        expect_eq(r.count, 2);
        expect_eq(r.ranges[0].start, 0);
        expect_eq(r.ranges[0].end, 300);
        expect_eq(r.ranges[1].start, 800);
        expect_eq(r.ranges[1].end, 900);
    });

    test("5. @cap/range-parser — Unsatisfiable ranges return RANGE_UNSATISFIABLE (-1)", () => {
        expect_eq(parseRange(1000, "bytes=1000-2000", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(1000, "bytes=500-100", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(1000, "bytes=-0", false).status, RANGE_UNSATISFIABLE);
        expect_eq(parseRange(0, "bytes=0-0", false).status, RANGE_UNSATISFIABLE);
    });

    test("6. @cap/range-parser — Malformed headers return RANGE_MALFORMED (-2)", () => {
        expect_eq(parseRange(1000, "invalid", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "bytes=-", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "bytes=abc-def", false).status, RANGE_MALFORMED);
        expect_eq(parseRange(1000, "=0-10", false).status, RANGE_MALFORMED);
    });
}
