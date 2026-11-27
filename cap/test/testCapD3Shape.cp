import { test, expect_eq } from "cap:test";
import {
    Point2D,
    PieSlice,
    computePie,
    linePath,
    stepLinePath,
    arcPath,
} from "../../packages/d3-shape/src/index.cp";

fn main() {
    test("1. @cap/d3-shape — computePie divides full circle proportionally", () => {
        const vals: [3]f64 = [1.0, 2.0, 1.0];
        let slices: [3]PieSlice = [
            PieSlice{ value: 0.0, index: 0, start_angle: 0.0, end_angle: 0.0, pad_angle: 0.0 },
            PieSlice{ value: 0.0, index: 0, start_angle: 0.0, end_angle: 0.0, pad_angle: 0.0 },
            PieSlice{ value: 0.0, index: 0, start_angle: 0.0, end_angle: 0.0, pad_angle: 0.0 },
        ];
        const n = computePie(vals[0..3], 0.0, 4.0, 0.05, slices[0..3]);
        expect_eq(n, 3 as usize);
        expect_eq(slices[0].start_angle, 0.0);
        expect_eq(slices[0].end_angle, 1.0);
        expect_eq(slices[1].start_angle, 1.0);
        expect_eq(slices[1].end_angle, 3.0);
        expect_eq(slices[2].start_angle, 3.0);
        expect_eq(slices[2].end_angle, 4.0);
        expect_eq(slices[2].pad_angle, 0.05);
    });

    test("2. @cap/d3-shape — linePath generates SVG M/L path", () => {
        const pts: [3]Point2D = [
            Point2D{ x: 0.0, y: 0.0 },
            Point2D{ x: 10.5, y: 20.0 },
            Point2D{ x: 30.0, y: -5.25 },
        ];
        let buf: [128]u8 = [0 as u8; 128];
        const n = linePath(pts[0..3], buf[0..128]);
        expect_eq(buf[0..n] as string, "M 0,0 L 10.5,20 L 30,-5.25");
    });

    test("3. @cap/d3-shape — stepLinePath generates SVG M/H/V step path", () => {
        const pts: [3]Point2D = [
            Point2D{ x: 0.0, y: 10.0 },
            Point2D{ x: 20.0, y: 30.0 },
            Point2D{ x: 50.0, y: 15.0 },
        ];
        let buf: [128]u8 = [0 as u8; 128];
        const n = stepLinePath(pts[0..3], buf[0..128]);
        expect_eq(buf[0..n] as string, "M 0,10 H 20 V 30 H 50 V 15");
    });

    test("4. @cap/d3-shape — arcPath generates quarter-circle sector (inner_r = 0)", () => {
        let buf: [256]u8 = [0 as u8; 256];
        const n = arcPath(0.0, 100.0, 0.0, std.math.pi / 2.0, buf[0..256]);
        expect_eq(buf[0..n] as string, "M 0,-100 A 100,100 0 0,1 100,0 L 0,0 Z");
    });

    test("5. @cap/d3-shape — arcPath handles large-arc donut ring (inner_r > 0)", () => {
        let buf: [256]u8 = [0 as u8; 256];
        const n = arcPath(50.0, 100.0, 0.0, 1.5 * std.math.pi, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "M 0,-100 A 100,100 0 1,1 -100,0 L -50,0 A 50,50 0 1,0 0,-50 Z",
        );
    });

    test("6. @cap/d3-shape — arcPath handles full circle and full annulus", () => {
        let buf: [256]u8 = [0 as u8; 256];
        const n = arcPath(0.0, 50.0, 0.0, 2.0 * std.math.pi, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "M 0,-50 A 50,50 0 1,1 0,50 A 50,50 0 1,1 0,-50 Z",
        );
    });
}
