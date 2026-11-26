import { test, expect_eq } from "cap:test";
import {
    Rgb,
    RgbColor,
    Hsv,
    Hwb,
    Hsl,
    Cmyk,
    rgbToHsv,
    hsvToRgb,
    rgbToHsl,
    hslToRgb,
    rgbToHwb,
    hwbToRgb,
    rgbToCmyk,
    cmykToRgb,
    rgbToAnsi256,
    ansi256ToRgb,
    rgbToAnsi16,
    ansi256ToAnsi16,
    ansi16ToRgb,
    rgbToHex,
    hexToRgb,
    keywordToRgb,
    rgbToKeyword,
    rgbToGray,
    grayToRgb,
} from "../../packages/@cap/color-convert/src/index.cp";

fn ccTestNear(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= eps;
}

fn ccTestStrEq(a: string, b: string): bool {
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

fn ccTestBufEq(buf: []u8, n: usize, expected: string): bool {
    if (n != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/color-convert — RGB <-> HSV and RGB <-> HWB round-trip precision", () => {
        // Pure primaries
        const redHsv: Hsv = rgbToHsv(255, 0, 0);
        expect_eq(ccTestNear(redHsv.h, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(redHsv.s, 100.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(redHsv.v, 100.0 as f64, 0.01 as f64), true);
        const redBack: Rgb = hsvToRgb(redHsv.h, redHsv.s, redHsv.v);
        expect_eq(redBack.r, 255);
        expect_eq(redBack.g, 0);
        expect_eq(redBack.b, 0);

        // Rich mid-tone orange (255, 136, 0)
        const orangeHsv: Hsv = rgbToHsv(255, 136, 0);
        expect_eq(ccTestNear(orangeHsv.h, 32.0 as f64, 0.05 as f64), true);
        expect_eq(ccTestNear(orangeHsv.s, 100.0 as f64, 0.05 as f64), true);
        expect_eq(ccTestNear(orangeHsv.v, 100.0 as f64, 0.05 as f64), true);
        const orangeBack: Rgb = hsvToRgb(orangeHsv.h, orangeHsv.s, orangeHsv.v);
        expect_eq(orangeBack.r, 255);
        expect_eq(orangeBack.g, 136);
        expect_eq(orangeBack.b, 0);

        // Cornflower blue (100, 149, 237) HSV + HWB + HSL round-trip
        const cbHsv: Hsv = rgbToHsv(100, 149, 237);
        const cbBack1: Rgb = hsvToRgb(cbHsv.h, cbHsv.s, cbHsv.v);
        expect_eq(cbBack1.r, 100);
        expect_eq(cbBack1.g, 149);
        expect_eq(cbBack1.b, 237);

        const cbHwb: Hwb = rgbToHwb(100, 149, 237);
        const cbBack2: Rgb = hwbToRgb(cbHwb.h, cbHwb.w, cbHwb.b);
        expect_eq(cbBack2.r, 100);
        expect_eq(cbBack2.g, 149);
        expect_eq(cbBack2.b, 237);

        const cbHsl: Hsl = rgbToHsl(100, 149, 237);
        const cbBack3: Rgb = hslToRgb(cbHsl.h, cbHsl.s, cbHsl.l);
        expect_eq(cbBack3.r, 100);
        expect_eq(cbBack3.g, 149);
        expect_eq(cbBack3.b, 237);

        // HWB achromatic white / black / gray + negative hue wrapping
        const whiteHwb: Hwb = rgbToHwb(255, 255, 255);
        expect_eq(ccTestNear(whiteHwb.w, 100.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(whiteHwb.b, 0.0 as f64, 0.01 as f64), true);
        const whiteBack: Rgb = hwbToRgb(whiteHwb.h, whiteHwb.w, whiteHwb.b);
        expect_eq(whiteBack.r, 255);
        expect_eq(whiteBack.g, 255);
        expect_eq(whiteBack.b, 255);

        const blackHwb: Hwb = rgbToHwb(0, 0, 0);
        expect_eq(ccTestNear(blackHwb.w, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(blackHwb.b, 100.0 as f64, 0.01 as f64), true);
        const blackBack: Rgb = hwbToRgb(blackHwb.h, blackHwb.w, blackHwb.b);
        expect_eq(blackBack.r, 0);
        expect_eq(blackBack.g, 0);
        expect_eq(blackBack.b, 0);

        const wrapRgb: Rgb = hsvToRgb(-120.0 as f64, 100.0 as f64, 100.0 as f64);
        expect_eq(wrapRgb.r, 0);
        expect_eq(wrapRgb.g, 0);
        expect_eq(wrapRgb.b, 255);
    });

    test("2. @cap/color-convert — RGB <-> CMYK (black, white, red, mixed) & Gray", () => {
        // Pure black
        const blk: Cmyk = rgbToCmyk(0, 0, 0);
        expect_eq(ccTestNear(blk.c, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(blk.m, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(blk.y, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(blk.k, 100.0 as f64, 0.01 as f64), true);
        const blkRgb: Rgb = cmykToRgb(blk.c, blk.m, blk.y, blk.k);
        expect_eq(blkRgb.r, 0);
        expect_eq(blkRgb.g, 0);
        expect_eq(blkRgb.b, 0);

        // Pure white
        const wht: Cmyk = rgbToCmyk(255, 255, 255);
        expect_eq(ccTestNear(wht.c, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(wht.m, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(wht.y, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(wht.k, 0.0 as f64, 0.01 as f64), true);
        const whtRgb: Rgb = cmykToRgb(wht.c, wht.m, wht.y, wht.k);
        expect_eq(whtRgb.r, 255);
        expect_eq(whtRgb.g, 255);
        expect_eq(whtRgb.b, 255);

        // Pure red -> (0, 100, 100, 0)
        const red: Cmyk = rgbToCmyk(255, 0, 0);
        expect_eq(ccTestNear(red.c, 0.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(red.m, 100.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(red.y, 100.0 as f64, 0.01 as f64), true);
        expect_eq(ccTestNear(red.k, 0.0 as f64, 0.01 as f64), true);
        const redRgb: Rgb = cmykToRgb(red.c, red.m, red.y, red.k);
        expect_eq(redRgb.r, 255);
        expect_eq(redRgb.g, 0);
        expect_eq(redRgb.b, 0);

        // Arbitrary mixed color round-trip
        const mix: Cmyk = rgbToCmyk(64, 192, 128);
        const mixRgb: Rgb = cmykToRgb(mix.c, mix.m, mix.y, mix.k);
        expect_eq(mixRgb.r, 64);
        expect_eq(mixRgb.g, 192);
        expect_eq(mixRgb.b, 128);

        // Grayscale round-trip
        const gPct: f64 = rgbToGray(128, 128, 128);
        const gRgb: Rgb = grayToRgb(gPct);
        expect_eq(gRgb.r, 128);
        expect_eq(gRgb.g, 128);
        expect_eq(gRgb.b, 128);
    });

    test("3. @cap/color-convert — RGB <-> ANSI256 (grayscale 232..255 and 6x6x6 cube)", () => {
        // Grayscale boundaries
        expect_eq(rgbToAnsi256(0, 0, 0), 16);
        expect_eq(rgbToAnsi256(7, 7, 7), 16);
        expect_eq(rgbToAnsi256(8, 8, 8), 232);
        expect_eq(rgbToAnsi256(248, 248, 248), 255);
        expect_eq(rgbToAnsi256(249, 249, 249), 231);
        expect_eq(rgbToAnsi256(255, 255, 255), 231);

        // Decode grayscale ramp 232..255
        const gStart: Rgb = ansi256ToRgb(232);
        expect_eq(gStart.r, 8);
        expect_eq(gStart.g, 8);
        expect_eq(gStart.b, 8);

        const gEnd: Rgb = ansi256ToRgb(255);
        expect_eq(gEnd.r, 238);
        expect_eq(gEnd.g, 238);
        expect_eq(gEnd.b, 238);

        // 6x6x6 color cube primaries
        expect_eq(rgbToAnsi256(255, 0, 0), 196);
        expect_eq(rgbToAnsi256(0, 255, 0), 46);
        expect_eq(rgbToAnsi256(0, 0, 255), 21);

        const c196: Rgb = ansi256ToRgb(196);
        expect_eq(c196.r, 255);
        expect_eq(c196.g, 0);
        expect_eq(c196.b, 0);

        // Exact cube round-trip for (102, 153, 204) -> index 110
        const idx110: u8 = rgbToAnsi256(102, 153, 204);
        expect_eq(idx110, 110);
        const back110: Rgb = ansi256ToRgb(idx110);
        expect_eq(back110.r, 102);
        expect_eq(back110.g, 153);
        expect_eq(back110.b, 204);

        // Low 16 standard colors decode safely without underflow
        const std9: Rgb = ansi256ToRgb(9);
        expect_eq(std9.r, 255);
        expect_eq(std9.g, 0);
        expect_eq(std9.b, 0);
    });

    test("4. @cap/color-convert — RGB -> ANSI16 and ANSI256 -> ANSI16 SGR codes", () => {
        // Dark / normal SGR 30..37
        expect_eq(rgbToAnsi16(0, 0, 0), 30);
        expect_eq(rgbToAnsi16(128, 0, 0), 31);
        expect_eq(rgbToAnsi16(0, 128, 0), 32);
        expect_eq(rgbToAnsi16(128, 128, 0), 33);
        expect_eq(rgbToAnsi16(0, 0, 128), 34);
        expect_eq(rgbToAnsi16(128, 0, 128), 35);
        expect_eq(rgbToAnsi16(0, 128, 128), 36);
        expect_eq(rgbToAnsi16(128, 128, 128), 37);

        // Bright SGR 90..97
        expect_eq(rgbToAnsi16(255, 0, 0), 91);
        expect_eq(rgbToAnsi16(0, 255, 0), 92);
        expect_eq(rgbToAnsi16(255, 255, 0), 93);
        expect_eq(rgbToAnsi16(0, 0, 255), 94);
        expect_eq(rgbToAnsi16(255, 0, 255), 95);
        expect_eq(rgbToAnsi16(0, 255, 255), 96);
        expect_eq(rgbToAnsi16(255, 255, 255), 97);

        // ANSI256 -> ANSI16
        expect_eq(ansi256ToAnsi16(16), 30);
        expect_eq(ansi256ToAnsi16(196), 91);
        expect_eq(ansi256ToAnsi16(46), 92);
        expect_eq(ansi256ToAnsi16(21), 94);
        expect_eq(ansi256ToAnsi16(231), 97);

        // ANSI16 -> RGB
        const a91: Rgb = ansi16ToRgb(91);
        expect_eq(a91.r, 255);
        expect_eq(a91.g, 0);
        expect_eq(a91.b, 0);
    });

    test("5. @cap/color-convert — Hex #RGB / #RRGGBB bidirectional codec", () => {
        let buf: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
        const n: usize = rgbToHex(255, 136, 0, buf[0..8]);
        expect_eq(n, 6);
        expect_eq(ccTestBufEq(buf[0..8], n, "FF8800"), true);

        const n2: usize = rgbToHex(10, 15, 160, buf[0..8]);
        expect_eq(n2, 6);
        expect_eq(ccTestBufEq(buf[0..8], n2, "0A0FA0"), true);

        // Insufficient output buffer returns 0
        let tiny: [4]u8 = [0, 0, 0, 0];
        expect_eq(rgbToHex(255, 0, 0, tiny[0..4]), 0);

        // #RRGGBB and RRGGBB (mixed case)
        const h1: RgbColor = hexToRgb("#FF8800");
        expect_eq(h1.found, true);
        expect_eq(h1.r, 255);
        expect_eq(h1.g, 136);
        expect_eq(h1.b, 0);

        const h2: RgbColor = hexToRgb("663399");
        expect_eq(h2.found, true);
        expect_eq(h2.r, 102);
        expect_eq(h2.g, 51);
        expect_eq(h2.b, 153);

        // #RGB and RGB shorthand
        const h3: RgbColor = hexToRgb("#f80");
        expect_eq(h3.found, true);
        expect_eq(h3.r, 255);
        expect_eq(h3.g, 136);
        expect_eq(h3.b, 0);

        const h4: RgbColor = hexToRgb("0aF");
        expect_eq(h4.found, true);
        expect_eq(h4.r, 0);
        expect_eq(h4.g, 170);
        expect_eq(h4.b, 255);

        // Invalid hex strings
        expect_eq(hexToRgb("").found, false);
        expect_eq(hexToRgb("#12345").found, false);
        expect_eq(hexToRgb("#GG0000").found, false);
    });

    test("6. @cap/color-convert — keywordToRgb / rgbToKeyword delegating to @cap/color-name", () => {
        const rp: RgbColor = keywordToRgb("rebeccapurple");
        expect_eq(rp.found, true);
        expect_eq(rp.r, 102);
        expect_eq(rp.g, 51);
        expect_eq(rp.b, 153);

        const cb: RgbColor = keywordToRgb("CornflowerBlue");
        expect_eq(cb.found, true);
        expect_eq(cb.r, 100);
        expect_eq(cb.g, 149);
        expect_eq(cb.b, 237);

        const miss: RgbColor = keywordToRgb("notarealcolor");
        expect_eq(miss.found, false);

        expect_eq(ccTestStrEq(rgbToKeyword(102, 51, 153), "rebeccapurple"), true);
        expect_eq(ccTestStrEq(rgbToKeyword(255, 99, 71), "tomato"), true);
        expect_eq(ccTestStrEq(rgbToKeyword(1, 2, 3), ""), true);
    });
}
