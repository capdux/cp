import { test, expect_eq } from "cap:test";
import {
    SvgColor,
    SvgCanvas,
} from "../../packages/@cap/plotters-svg/src/index.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/plotters-svg — begin() & end() emit valid SVG root wrapper", () => {
        let buf: [256]u8 = [0 as u8; 256];
        let canvas: SvgCanvas = SvgCanvas.new(buf[0..256]);
        expect_eq(canvas.begin(800, 600), true);
        const total = canvas.end();
        expect_eq(total > 0, true);
        expect_eq(canvas.hasOverflow(), false);
        const svg = canvas.slice();
        expect_eq(containsSubstr(svg, "<svg width=\"800\" height=\"600\" viewBox=\"0 0 800 600\""), true);
        expect_eq(containsSubstr(svg, "</svg>"), true);
    });

    test("2. @cap/plotters-svg — drawRect with fill & optional stroke_width", () => {
        let buf: [512]u8 = [0 as u8; 512];
        let canvas: SvgCanvas = SvgCanvas.new(buf[0..512]);
        expect_eq(canvas.begin(400, 300), true);
        expect_eq(canvas.drawRect(10, 20, 100, 50, SvgColor.rgb(255, 128, 0), 0), true);
        expect_eq(canvas.drawRect(-5, 0, 40, 40, SvgColor.blue(), 2), true);
        * = canvas.end();
        const svg = canvas.slice();
        expect_eq(containsSubstr(svg, "<rect x=\"10\" y=\"20\" width=\"100\" height=\"50\" fill=\"rgb(255,128,0)\"/>"), true);
        expect_eq(containsSubstr(svg, "<rect x=\"-5\" y=\"0\" width=\"40\" height=\"40\" fill=\"rgb(0,0,255)\" stroke=\"rgb(0,0,0)\" stroke-width=\"2\"/>"), true);
    });

    test("3. @cap/plotters-svg — drawLine & drawCircle", () => {
        let buf: [512]u8 = [0 as u8; 512];
        let canvas: SvgCanvas = SvgCanvas.new(buf[0..512]);
        expect_eq(canvas.begin(200, 200), true);
        expect_eq(canvas.drawLine(0, 100, 200, 100, SvgColor.red(), 3), true);
        expect_eq(canvas.drawCircle(100, 100, 25, SvgColor.green()), true);
        * = canvas.end();
        const svg = canvas.slice();
        expect_eq(containsSubstr(svg, "<line x1=\"0\" y1=\"100\" x2=\"200\" y2=\"100\" stroke=\"rgb(255,0,0)\" stroke-width=\"3\"/>"), true);
        expect_eq(containsSubstr(svg, "<circle cx=\"100\" cy=\"100\" r=\"25\" fill=\"rgb(0,128,0)\"/>"), true);
    });

    test("4. @cap/plotters-svg — drawPolyline multi-point series", () => {
        let buf: [512]u8 = [0 as u8; 512];
        let canvas: SvgCanvas = SvgCanvas.new(buf[0..512]);
        expect_eq(canvas.begin(300, 200), true);
        const xs: [4]i32 = [0 as i32, 50 as i32, 100 as i32, 150 as i32];
        const ys: [4]i32 = [180 as i32, 120 as i32, 140 as i32, 40 as i32];
        expect_eq(canvas.drawPolyline(xs[0..4], ys[0..4], SvgColor.rgb(30, 144, 255), 2), true);
        * = canvas.end();
        const svg = canvas.slice();
        expect_eq(containsSubstr(svg, "<polyline fill=\"none\" stroke=\"rgb(30,144,255)\" stroke-width=\"2\" points=\"0,180 50,120 100,140 150,40\"/>"), true);
    });

    test("5. @cap/plotters-svg — drawText automatic XML escaping (<>&\")", () => {
        let buf: [512]u8 = [0 as u8; 512];
        let canvas: SvgCanvas = SvgCanvas.new(buf[0..512]);
        expect_eq(canvas.begin(400, 100), true);
        expect_eq(canvas.drawText(12, 34, "A < B & \"C\" > D", 16, SvgColor.black()), true);
        * = canvas.end();
        const svg = canvas.slice();
        expect_eq(containsSubstr(svg, "<text x=\"12\" y=\"34\" font-size=\"16\" fill=\"rgb(0,0,0)\">A &lt; B &amp; &quot;C&quot; &gt; D</text>"), true);
    });

    test("6. @cap/plotters-svg — tiny buffer gracefully flags overflow without OOB", () => {
        let tiny: [32]u8 = [0 as u8; 32];
        let canvas: SvgCanvas = SvgCanvas.new(tiny[0..32]);
        expect_eq(canvas.begin(800, 600), false);
        expect_eq(canvas.hasOverflow(), true);
        expect_eq(canvas.written(), 32 as usize);
    });
}
