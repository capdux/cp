// cap/test/testCapColorQuant.cp
// Comprehensive unit tests for @cap/color-quant

import { test, expect_eq } from "cap:test";
import {
    Rgb8,
    ColorPalette,
    buildUniformPalette,
    quantizeHistogram,
    mapPixelsToPalette,
} from "../../packages/@cap/color-quant/src/index.cp";

fn main() {
    test("1. @cap/color-quant — buildUniformPalette produces 256 colors (6x6x6 cube + 40 grays)", () => {
        const pal = buildUniformPalette();
        expect_eq(pal.len, 256 as usize);

        // Index 0 is black (0,0,0), index 215 is white (255,255,255)
        expect_eq(pal.colors[0].r, 0 as u8);
        expect_eq(pal.colors[0].g, 0 as u8);
        expect_eq(pal.colors[0].b, 0 as u8);

        expect_eq(pal.colors[215].r, 255 as u8);
        expect_eq(pal.colors[215].g, 255 as u8);
        expect_eq(pal.colors[215].b, 255 as u8);

        // Grayscale ramp at indices 216..255 has r == g == b and increases monotonically
        expect_eq(pal.colors[216].r, pal.colors[216].g);
        expect_eq(pal.colors[216].g, pal.colors[216].b);
        expect_eq(pal.colors[255].r > pal.colors[216].r, true);
    });

    test("2. @cap/color-quant — nearestIndex finds exact and closest Euclidean RGB match", () => {
        const pal = buildUniformPalette();

        // Exact matches for pure black, white, red (5*36=180), green (5*6=30), blue (5)
        expect_eq(pal.nearestIndex(Rgb8{ r: 0, g: 0, b: 0 }), 0 as u8);
        expect_eq(pal.nearestIndex(Rgb8{ r: 255, g: 255, b: 255 }), 215 as u8);
        expect_eq(pal.nearestIndex(Rgb8{ r: 255, g: 0, b: 0 }), 180 as u8);
        expect_eq(pal.nearestIndex(Rgb8{ r: 0, g: 255, b: 0 }), 30 as u8);
        expect_eq(pal.nearestIndex(Rgb8{ r: 0, g: 0, b: 255 }), 5 as u8);

        // Near-red pixel maps to pure red (180)
        expect_eq(pal.nearestIndex(Rgb8{ r: 250, g: 3, b: 2 }), 180 as u8);
    });

    test("3. @cap/color-quant — quantizeHistogram aggregates 3-3-2 buckets and extracts mean colors", () => {
        let pixels: [6]Rgb8 = undefined;
        // Bucket 1: 4 reddish pixels (240..248 -> same top 3 bits = 7)
        pixels[0] = Rgb8{ r: 240, g: 10, b: 20 };
        pixels[1] = Rgb8{ r: 244, g: 14, b: 24 };
        pixels[2] = Rgb8{ r: 248, g: 18, b: 28 };
        pixels[3] = Rgb8{ r: 244, g: 14, b: 24 };
        // Bucket 2: 2 blueish pixels
        pixels[4] = Rgb8{ r: 10, g: 20, b: 220 };
        pixels[5] = Rgb8{ r: 14, g: 24, b: 224 };

        const pal = quantizeHistogram(pixels[0..6], 16);
        expect_eq(pal.len, 2 as usize);

        // Most frequent bucket (reddish) comes first with mean (244, 14, 24)
        expect_eq(pal.colors[0].r, 244 as u8);
        expect_eq(pal.colors[0].g, 14 as u8);
        expect_eq(pal.colors[0].b, 24 as u8);

        // Second bucket (blueish) comes second with mean (12, 22, 222)
        expect_eq(pal.colors[1].r, 12 as u8);
        expect_eq(pal.colors[1].g, 22 as u8);
        expect_eq(pal.colors[1].b, 222 as u8);
    });

    test("4. @cap/color-quant — quantizeHistogram respects max_colors cap", () => {
        let pixels: [6]Rgb8 = undefined;
        pixels[0] = Rgb8{ r: 255, g: 0, b: 0 };
        pixels[1] = Rgb8{ r: 255, g: 0, b: 0 };
        pixels[2] = Rgb8{ r: 255, g: 0, b: 0 };
        pixels[3] = Rgb8{ r: 0, g: 255, b: 0 };
        pixels[4] = Rgb8{ r: 0, g: 255, b: 0 };
        pixels[5] = Rgb8{ r: 0, g: 0, b: 255 };

        const pal = quantizeHistogram(pixels[0..6], 2);
        expect_eq(pal.len, 2 as usize);
        // Top 2 are red (count 3) and green (count 2)
        expect_eq(pal.colors[0].r, 255 as u8);
        expect_eq(pal.colors[1].g, 255 as u8);
    });

    test("5. @cap/color-quant — mapPixelsToPalette maps pixels and respects output buffer length", () => {
        let pixels: [4]Rgb8 = undefined;
        pixels[0] = Rgb8{ r: 255, g: 0, b: 0 };
        pixels[1] = Rgb8{ r: 0, g: 255, b: 0 };
        pixels[2] = Rgb8{ r: 0, g: 0, b: 255 };
        pixels[3] = Rgb8{ r: 250, g: 5, b: 5 };

        const pal = quantizeHistogram(pixels[0..4], 3);
        let out: [4]u8 = undefined;
        const mapped = mapPixelsToPalette(pixels[0..4], pal, out[0..4]);
        expect_eq(mapped, 4 as usize);
        // Pixels 0 and 3 both map to the same red entry (index 0)
        expect_eq(out[0], 0 as u8);
        expect_eq(out[3], 0 as u8);
        // Pixel 1 and 2 map to distinct non-zero entries
        expect_eq(out[1] != out[0], true);
        expect_eq(out[2] != out[0], true);
        expect_eq(out[1] != out[2], true);

        // Smaller output buffer clamps mapped count
        let smallOut: [2]u8 = undefined;
        expect_eq(mapPixelsToPalette(pixels[0..4], pal, smallOut[0..2]), 2 as usize);
    });

    test("6. @cap/color-quant — Empty input and zero max_colors edge cases", () => {
        let onePx: [1]Rgb8 = undefined;
        onePx[0] = Rgb8{ r: 42, g: 84, b: 126 };

        const p1 = quantizeHistogram(onePx[0..0], 16);
        expect_eq(p1.len, 0 as usize);
        expect_eq(p1.nearestIndex(Rgb8{ r: 100, g: 100, b: 100 }), 0 as u8);

        const p2 = quantizeHistogram(onePx[0..1], 0);
        expect_eq(p2.len, 0 as usize);

        let out: [1]u8 = undefined;
        expect_eq(mapPixelsToPalette(onePx[0..1], p2, out[0..1]), 0 as usize);
    });
}
