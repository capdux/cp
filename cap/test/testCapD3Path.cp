import { test, expect_eq } from "cap:test";
import { SvgPath } from "../../packages/d3-path/src/index.cp";

fn main() {
    test("1. @cap/d3-path — moveTo, lineTo, and closePath build triangle", () => {
        let p = SvgPath.new();
        p.moveTo(0.0, 0.0);
        p.lineTo(10.0, 0.0);
        p.lineTo(5.0, 10.0);
        p.closePath();
        let out: [128]u8 = undefined;
        let n = p.writeTo(out[0..128]);
        expect_eq(out[0..n], "M0,0L10,0L5,10Z");
    });

    test("2. @cap/d3-path — quadraticCurveTo formats fractional coordinates cleanly", () => {
        let p = SvgPath.new();
        p.moveTo(1.5, 2.25);
        p.quadraticCurveTo(3.0, 4.5, 5.125, -6.0);
        let out: [128]u8 = undefined;
        let n = p.writeTo(out[0..128]);
        expect_eq(out[0..n], "M1.5,2.25Q3,4.5,5.125,-6");
    });

    test("3. @cap/d3-path — bezierCurveTo serializes cubic control points", () => {
        let p = SvgPath.new();
        p.moveTo(0.0, 0.0);
        p.bezierCurveTo(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
        let out: [128]u8 = undefined;
        let n = p.writeTo(out[0..128]);
        expect_eq(out[0..n], "M0,0C1,2,3,4,5,6");
    });

    test("4. @cap/d3-path — rect emits M..h..v..h..Z", () => {
        let p = SvgPath.new();
        p.rect(10.0, 20.0, 30.0, 40.0);
        let out: [128]u8 = undefined;
        let n = p.writeTo(out[0..128]);
        expect_eq(out[0..n], "M10,20h30v40h-30Z");
    });

    test("5. @cap/d3-path — closePath without subpath is a no-op", () => {
        let p = SvgPath.new();
        p.closePath();
        expect_eq(p.byteLen(), 0 as usize);
    });

    test("6. @cap/d3-path — reset clears buffer and state", () => {
        let p = SvgPath.new();
        p.moveTo(1.0, 2.0);
        p.reset();
        expect_eq(p.byteLen(), 0 as usize);
        p.moveTo(9.0, 8.0);
        let out: [64]u8 = undefined;
        let n = p.writeTo(out[0..64]);
        expect_eq(out[0..n], "M9,8");
    });
}
