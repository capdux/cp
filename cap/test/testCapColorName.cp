import { test, expect_eq } from "cap:test";
import {
    CSS_COLOR_COUNT,
    RgbColor,
    ColorEntry,
    colorAt,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
    isNamedColor,
} from "../../packages/color-name/src/index.cp";

fn cnTestEq(a: string, b: string): bool {
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

fn cnTestLess(a: string, b: string): bool {
    let min_len: usize = a.len;
    if (b.len < min_len) {
        min_len = b.len;
    }
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) { return true; }
        if (a[i] > b[i]) { return false; }
        i += 1;
    }
    return a.len < b.len;
}

fn main() {
    test("1. @cap/color-name — Core CSS colors (red, green, blue, white, black, rebeccapurple)", () => {
        expect_eq(CSS_COLOR_COUNT, 148);

        const red: RgbColor = lookupColor("red");
        expect_eq(red.found, true);
        expect_eq(red.r, 255);
        expect_eq(red.g, 0);
        expect_eq(red.b, 0);

        const green: RgbColor = lookupColor("green");
        expect_eq(green.found, true);
        expect_eq(green.r, 0);
        expect_eq(green.g, 128);
        expect_eq(green.b, 0);

        const blue: RgbColor = lookupColor("blue");
        expect_eq(blue.found, true);
        expect_eq(blue.b, 255);

        const white: RgbColor = lookupColor("white");
        expect_eq(white.r, 255);
        expect_eq(white.g, 255);
        expect_eq(white.b, 255);

        const black: RgbColor = lookupColor("black");
        expect_eq(black.found, true);
        expect_eq(black.r, 0);
        expect_eq(black.g, 0);
        expect_eq(black.b, 0);

        const rp: RgbColor = lookupColor("rebeccapurple");
        expect_eq(rp.found, true);
        expect_eq(rp.r, 102);
        expect_eq(rp.g, 51);
        expect_eq(rp.b, 153);
    });

    test("2. @cap/color-name — Case-insensitive binary lookup", () => {
        const c1: RgbColor = lookupColor("CornflowerBlue");
        expect_eq(c1.found, true);
        expect_eq(c1.r, 100);
        expect_eq(c1.g, 149);
        expect_eq(c1.b, 237);

        const c2: RgbColor = lookupColor("ALICEBLUE");
        expect_eq(c2.found, true);
        expect_eq(c2.r, 240);
        expect_eq(c2.g, 248);
        expect_eq(c2.b, 255);

        const c3: RgbColor = lookupColor("YellowGreen");
        expect_eq(c3.found, true);
        expect_eq(c3.r, 154);
        expect_eq(c3.g, 205);
        expect_eq(c3.b, 50);
    });

    test("3. @cap/color-name — Alias parity (gray/grey, aqua/cyan, fuchsia/magenta)", () => {
        const g1: RgbColor = lookupColor("gray");
        const g2: RgbColor = lookupColor("grey");
        expect_eq(g1.found, true);
        expect_eq(g2.found, true);
        expect_eq(g1.r, g2.r);
        expect_eq(g1.g, g2.g);
        expect_eq(g1.b, g2.b);

        const dsg1: RgbColor = lookupColor("darkslategray");
        const dsg2: RgbColor = lookupColor("darkslategrey");
        expect_eq(dsg1.r, dsg2.r);
        expect_eq(dsg1.g, dsg2.g);
        expect_eq(dsg1.b, dsg2.b);

        expect_eq(lookupColorHex("aqua"), lookupColorHex("cyan"));
        expect_eq(lookupColorHex("fuchsia"), lookupColorHex("magenta"));
    });

    test("4. @cap/color-name — lookupColorHex and reverseLookupColor round-trip", () => {
        expect_eq(lookupColorHex("rebeccapurple"), 0x663399);
        expect_eq(lookupColorHex("tomato"), 0xFF6347);
        expect_eq(lookupColorHex("nonexistent"), -1);

        expect_eq(cnTestEq(reverseLookupColor(102, 51, 153), "rebeccapurple"), true);
        expect_eq(cnTestEq(reverseLookupColor(255, 99, 71), "tomato"), true);
        expect_eq(cnTestEq(reverseLookupColor(1, 2, 3), ""), true);
    });

    test("5. @cap/color-name — Full 148-entry table strict sort order and binary-search self-check", () => {
        let i: usize = 0;
        while (i < CSS_COLOR_COUNT) {
            const cur: ColorEntry = colorAt(i);
            if (i > 0) {
                const prev: ColorEntry = colorAt(i - 1);
                expect_eq(cnTestLess(prev.name, cur.name), true);
            }
            const res: RgbColor = lookupColor(cur.name);
            expect_eq(res.found, true);
            expect_eq(res.r, cur.r);
            expect_eq(res.g, cur.g);
            expect_eq(res.b, cur.b);
            i += 1;
        }
    });

    test("6. @cap/color-name — Unknown color names and empty string safety", () => {
        expect_eq(isNamedColor("crimson"), true);
        expect_eq(isNamedColor(""), false);
        expect_eq(isNamedColor("notacolor"), false);
        expect_eq(isNamedColor("red "), false);

        const miss: RgbColor = lookupColor("ultraviolet");
        expect_eq(miss.found, false);
        expect_eq(miss.r, 0);
        expect_eq(miss.g, 0);
        expect_eq(miss.b, 0);
    });
}
