// cap/test/testCapSvgPath.cp
// Comprehensive unit tests for @cap/svg-path

import { test, expect_eq } from "cap:test";
import {
    PathCmd,
    ParsedPath,
    PathBounds,
    parseSvgPath,
    computeBounds,
    formatSvgPath,
} from "../../packages/parse-svg-path/src/index.cp";

fn approxEq(a: f64, b: f64) -> bool {
    const diff = if (a > b) a - b else b - a;
    return diff < 0.0001;
}

fn main() {
    test("1. @cap/svg-path — Basic commands (M, L, H, V, Z)", () => {
        const p = parseSvgPath("M 0 0 L 100 0 V 50 H 0 Z");
        expect_eq(p.valid, true);
        expect_eq(p.count, 5 as usize);
        expect_eq(p.cmdAt(0).?.code, 'M' as u8);
        expect_eq(p.cmdAt(1).?.code, 'L' as u8);
        expect_eq(p.cmdAt(2).?.code, 'V' as u8);
        expect_eq(p.cmdAt(3).?.code, 'H' as u8);
        expect_eq(p.cmdAt(4).?.code, 'Z' as u8);
        expect_eq(p.cmdAt(4).?.arg_count, 0 as u8);
    });

    test("2. @cap/svg-path — Compact signs (M10-20), shorthand decimals (.5.5) & scientific (1e-3)", () => {
        const p = parseSvgPath("M10-20L.5.5l1e-3-2.5e1");
        expect_eq(p.valid, true);
        expect_eq(p.count, 3 as usize);

        const c0 = p.cmdAt(0).?;
        expect_eq(approxEq(c0.args[0], 10.0), true);
        expect_eq(approxEq(c0.args[1], -20.0), true);

        const c1 = p.cmdAt(1).?;
        expect_eq(approxEq(c1.args[0], 0.5), true);
        expect_eq(approxEq(c1.args[1], 0.5), true);

        const c2 = p.cmdAt(2).?;
        expect_eq(c2.relative, true);
        expect_eq(approxEq(c2.args[0], 0.001), true);
        expect_eq(approxEq(c2.args[1], -25.0), true);
    });

    test("3. @cap/svg-path — Implicit repeated commands (M->L, m->l)", () => {
        const p1 = parseSvgPath("M 0 0 10 20 30 40");
        expect_eq(p1.valid, true);
        expect_eq(p1.count, 3 as usize);
        expect_eq(p1.cmdAt(0).?.code, 'M' as u8);
        expect_eq(p1.cmdAt(1).?.code, 'L' as u8);
        expect_eq(p1.cmdAt(2).?.code, 'L' as u8);
        expect_eq(approxEq(p1.cmdAt(2).?.args[0], 30.0), true);
        expect_eq(approxEq(p1.cmdAt(2).?.args[1], 40.0), true);

        const p2 = parseSvgPath("m 5 5 10 0 0 10");
        expect_eq(p2.valid, true);
        expect_eq(p2.count, 3 as usize);
        expect_eq(p2.cmdAt(1).?.code, 'l' as u8);
        expect_eq(p2.cmdAt(1).?.relative, true);
        expect_eq(p2.cmdAt(2).?.code, 'l' as u8);
    });

    test("4. @cap/svg-path — Relative coordinate bounding box with H/V/Z", () => {
        const p = parseSvgPath("M 10 20 h 30 v 40 l -50 -10 z");
        const b = computeBounds(p);
        expect_eq(b.valid, true);
        // Points: (10,20) -> (40,20) -> (40,60) -> (-10,50) -> Z(10,20)
        expect_eq(approxEq(b.min_x, -10.0), true);
        expect_eq(approxEq(b.min_y, 20.0), true);
        expect_eq(approxEq(b.max_x, 40.0), true);
        expect_eq(approxEq(b.max_y, 60.0), true);
        expect_eq(approxEq(b.width, 50.0), true);
        expect_eq(approxEq(b.height, 40.0), true);
    });

    test("5. @cap/svg-path — Cubic/Quadratic Bezier & Arc (A) compact flags", () => {
        const p = parseSvgPath("M 0 0 C 10 20 30 40 50 0 Q 60 -20 80 0 A 25 25 0 11 100 50");
        expect_eq(p.valid, true);
        expect_eq(p.count, 4 as usize);

        const arc = p.cmdAt(3).?;
        expect_eq(arc.code, 'A' as u8);
        expect_eq(arc.arg_count, 7 as u8);
        expect_eq(approxEq(arc.args[0], 25.0), true);
        expect_eq(approxEq(arc.args[3], 1.0), true); // large-arc-flag
        expect_eq(approxEq(arc.args[4], 1.0), true); // sweep-flag
        expect_eq(approxEq(arc.args[5], 100.0), true);
        expect_eq(approxEq(arc.args[6], 50.0), true);

        const b = computeBounds(p);
        expect_eq(approxEq(b.min_x, 0.0), true);
        expect_eq(approxEq(b.min_y, -20.0), true);
        expect_eq(approxEq(b.max_x, 100.0), true);
        expect_eq(approxEq(b.max_y, 50.0), true);
    });

    test("6. @cap/svg-path — formatSvgPath canonical output & invalid path rejection", () => {
        let out: [256]u8 = undefined;
        const p = parseSvgPath("M10-20 L.5,.5 Z");
        expect_eq(formatSvgPath(p, out[0..256]), "M 10 -20 L 0.5 0.5 Z");

        // Invalid paths
        expect_eq(parseSvgPath("").valid, false);
        expect_eq(parseSvgPath("X 10 20").valid, false);
        expect_eq(parseSvgPath("M 10").valid, false);
        expect_eq(parseSvgPath("A 10 10 0 2 0 50 50").valid, false); // invalid arc flag '2'
        expect_eq(parseSvgPath("Z 10 20").valid, false);
    });
}
