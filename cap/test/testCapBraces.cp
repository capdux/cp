// cap/test/testCapBraces.cp
// Comprehensive unit tests for @cap/braces

import { test, expect_eq } from "cap:test";
import {
    expandBraces,
    fillRange,
} from "../../packages/braces/src/index.cp";

fn main() {
    test("1. @cap/braces — Comma set expansion & empty alternatives", () => {
        let out1: [512]u8 = undefined;
        const r1 = expandBraces("a/{b,c,d}/e", out1[0..512]);
        expect_eq(r1.count, 3 as usize);
        expect_eq(r1.itemAt(0 as usize), "a/b/e");
        expect_eq(r1.itemAt(1 as usize), "a/c/e");
        expect_eq(r1.itemAt(2 as usize), "a/d/e");

        let out2: [512]u8 = undefined;
        const r2 = expandBraces("a{,b}", out2[0..512]);
        expect_eq(r2.count, 2 as usize);
        expect_eq(r2.itemAt(0 as usize), "a");
        expect_eq(r2.itemAt(1 as usize), "ab");
    });

    test("2. @cap/braces — Numeric ranges, step, zero-padding & negatives", () => {
        let out1: [512]u8 = undefined;
        const r1 = expandBraces("{1..5}", out1[0..512]);
        expect_eq(r1.count, 5 as usize);
        expect_eq(r1.itemAt(0 as usize), "1");
        expect_eq(r1.itemAt(4 as usize), "5");

        let out2: [512]u8 = undefined;
        const r2 = expandBraces("{10..2..2}", out2[0..512]);
        expect_eq(r2.count, 5 as usize);
        expect_eq(r2.itemAt(0 as usize), "10");
        expect_eq(r2.itemAt(1 as usize), "8");
        expect_eq(r2.itemAt(4 as usize), "2");

        let out3: [512]u8 = undefined;
        const r3 = expandBraces("{01..03}", out3[0..512]);
        expect_eq(r3.count, 3 as usize);
        expect_eq(r3.itemAt(0 as usize), "01");
        expect_eq(r3.itemAt(1 as usize), "02");
        expect_eq(r3.itemAt(2 as usize), "03");

        let out4: [512]u8 = undefined;
        const r4 = expandBraces("{-2..1}", out4[0..512]);
        expect_eq(r4.count, 4 as usize);
        expect_eq(r4.itemAt(0 as usize), "-2");
        expect_eq(r4.itemAt(1 as usize), "-1");
        expect_eq(r4.itemAt(2 as usize), "0");
        expect_eq(r4.itemAt(3 as usize), "1");
    });

    test("3. @cap/braces — Alphabetic ranges & direct fillRange API", () => {
        let out1: [512]u8 = undefined;
        const r1 = expandBraces("{a..e}", out1[0..512]);
        expect_eq(r1.count, 5 as usize);
        expect_eq(r1.itemAt(0 as usize), "a");
        expect_eq(r1.itemAt(4 as usize), "e");

        let out2: [512]u8 = undefined;
        const r2 = expandBraces("{Z..W}", out2[0..512]);
        expect_eq(r2.count, 4 as usize);
        expect_eq(r2.itemAt(0 as usize), "Z");
        expect_eq(r2.itemAt(1 as usize), "Y");
        expect_eq(r2.itemAt(2 as usize), "X");
        expect_eq(r2.itemAt(3 as usize), "W");

        let out3: [512]u8 = undefined;
        const r3 = fillRange("a", "g", 2 as i32, out3[0..512]);
        expect_eq(r3.count, 4 as usize);
        expect_eq(r3.itemAt(0 as usize), "a");
        expect_eq(r3.itemAt(1 as usize), "c");
        expect_eq(r3.itemAt(2 as usize), "e");
        expect_eq(r3.itemAt(3 as usize), "g");
    });

    test("4. @cap/braces — Cartesian product expansion", () => {
        let out: [512]u8 = undefined;
        const r = expandBraces("{x,y}{1..2}", out[0..512]);
        expect_eq(r.count, 4 as usize);
        expect_eq(r.itemAt(0 as usize), "x1");
        expect_eq(r.itemAt(1 as usize), "x2");
        expect_eq(r.itemAt(2 as usize), "y1");
        expect_eq(r.itemAt(3 as usize), "y2");
    });

    test("5. @cap/braces — Nested brace expansion", () => {
        let out: [512]u8 = undefined;
        const r = expandBraces("src/{a,b/{c,d}}.cp", out[0..512]);
        expect_eq(r.count, 3 as usize);
        expect_eq(r.itemAt(0 as usize), "src/a.cp");
        expect_eq(r.itemAt(1 as usize), "src/b/c.cp");
        expect_eq(r.itemAt(2 as usize), "src/b/d.cp");
    });

    test("6. @cap/braces — Unclosed/non-expandable passthrough & explosion guard", () => {
        let out1: [256]u8 = undefined;
        const r1 = expandBraces("a{b", out1[0..256]);
        expect_eq(r1.count, 1 as usize);
        expect_eq(r1.itemAt(0 as usize), "a{b");

        let out2: [256]u8 = undefined;
        const r2 = expandBraces("a{only}b", out2[0..256]);
        expect_eq(r2.count, 1 as usize);
        expect_eq(r2.itemAt(0 as usize), "a{only}b");

        let out3: [1024]u8 = undefined;
        const r3 = expandBraces("{1..100}", out3[0..1024]);
        expect_eq(r3.count, 64 as usize);
        expect_eq(r3.truncated, true);
        expect_eq(r3.itemAt(0 as usize), "1");
        expect_eq(r3.itemAt(63 as usize), "64");
        expect_eq(r3.itemAt(99 as usize), "");
    });
}
