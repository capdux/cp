// cap/test/testCapNumRational.cp
// Comprehensive unit tests for @cap/num-rational

import { test, expect_eq } from "cap:test";
import {
    Ratio64,
    parseRatio,
    formatRatio,
} from "../../packages/@cap/num-rational/src/index.cp";

fn main() {
    test("1. @cap/num-rational — GCD reduction, sign normalization & zero denominator rejection", () => {
        const r1 = Ratio64.new(-6, -9).?;
        expect_eq(r1.numer, 2 as i64);
        expect_eq(r1.denom, 3 as i64);

        const r2 = Ratio64.new(6, -9).?;
        expect_eq(r2.numer, -2 as i64);
        expect_eq(r2.denom, 3 as i64);

        const r0 = Ratio64.new(0, 5).?;
        expect_eq(r0.numer, 0 as i64);
        expect_eq(r0.denom, 1 as i64);
        expect_eq(r0.isInteger(), true);

        expect_eq(Ratio64.new(1, 0) == null, true);
    });

    test("2. @cap/num-rational — Exact arithmetic (add, sub, mul, div, neg, abs, recip)", () => {
        const a = Ratio64.new(1, 3).?;
        const b = Ratio64.new(1, 6).?;

        const sum = a.add(b);
        expect_eq(sum.equals(Ratio64.new(1, 2).?), true);

        const diff = b.sub(a);
        const diff_abs = diff.abs();
        expect_eq(diff.equals(Ratio64.new(-1, 6).?), true);
        expect_eq(diff_abs.equals(Ratio64.new(1, 6).?), true);

        const r34 = Ratio64.new(3, 4).?;
        const prod = r34.mul(Ratio64.new(8, 9).?);
        expect_eq(prod.equals(Ratio64.new(2, 3).?), true);

        const r23 = Ratio64.new(2, 3).?;
        const quot = r23.div(Ratio64.new(4, 5).?).?;
        expect_eq(quot.equals(Ratio64.new(5, 6).?), true);

        const z = Ratio64.zero();
        expect_eq(a.div(z) == null, true);
        expect_eq(z.recip() == null, true);
        const neg35 = Ratio64.new(-3, 5).?;
        const inv35 = neg35.recip().?;
        expect_eq(inv35.equals(Ratio64.new(-5, 3).?), true);
    });

    test("3. @cap/num-rational — Exact comparison (cmp)", () => {
        const a = Ratio64.new(2, 3).?;
        const b = Ratio64.new(3, 4).?;
        const neg_a = Ratio64.new(-2, 3).?;
        const neg_b = Ratio64.new(-3, 4).?;

        expect_eq(a.cmp(b), -1);
        expect_eq(b.cmp(a), 1);
        expect_eq(a.cmp(Ratio64.new(4, 6).?), 0);
        expect_eq(neg_b.cmp(neg_a), -1);
        expect_eq(neg_a.cmp(a), -1);
    });

    test("4. @cap/num-rational — floor, ceil, trunc & toF64 (positive & negative)", () => {
        const pos = Ratio64.new(7, 3).?; // 2.333...
        expect_eq(pos.trunc(), 2 as i64);
        expect_eq(pos.floor(), 2 as i64);
        expect_eq(pos.ceil(), 3 as i64);

        const neg = Ratio64.new(-7, 3).?; // -2.333...
        expect_eq(neg.trunc(), -2 as i64);
        expect_eq(neg.floor(), -3 as i64);
        expect_eq(neg.ceil(), -2 as i64);

        const exact = Ratio64.new(-6, 3).?; // -2
        expect_eq(exact.trunc(), -2 as i64);
        expect_eq(exact.floor(), -2 as i64);
        expect_eq(exact.ceil(), -2 as i64);

        const ntsc = Ratio64.new(30000, 1001).?;
        const fps = ntsc.toF64();
        expect_eq(fps > 29.97 && fps < 29.971, true);
    });

    test("5. @cap/num-rational — parseRatio & formatRatio round-trip", () => {
        let out: [64]u8 = undefined;

        const r1 = parseRatio("3/4").?;
        expect_eq(formatRatio(r1, out[0..64]), "3/4");

        const r2 = parseRatio("-6/8").?;
        expect_eq(formatRatio(r2, out[0..64]), "-3/4");

        const r3 = parseRatio("42").?;
        expect_eq(formatRatio(r3, out[0..64]), "42");

        const r4 = parseRatio("10/-2").?;
        expect_eq(formatRatio(r4, out[0..64]), "-5");

        expect_eq(parseRatio("1/0") == null, true);
        expect_eq(parseRatio("abc") == null, true);
        expect_eq(parseRatio("") == null, true);
    });

    test("6. @cap/num-rational — Financial & NTSC frame-rate exact identity (0.1 + 0.2 == 0.3)", () => {
        const r01 = Ratio64.new(1, 10).?;
        const r02 = Ratio64.new(2, 10).?;
        const r03 = Ratio64.new(3, 10).?;
        const sum03 = r01.add(r02);
        expect_eq(sum03.equals(r03), true);

        // 1001 frames at 1001/30000 sec/frame == exact 1001/30 seconds
        const frame_dur = Ratio64.new(1001, 30000).?;
        const total = frame_dur.mul(Ratio64.fromInt(30000));
        expect_eq(total.equals(Ratio64.fromInt(1001)), true);
    });
}
