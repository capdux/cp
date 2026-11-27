// cap/test/testCapColorNames.cp
// Comprehensive unit tests for @cap/color-name (CSS Color Module Level 4)

import { test, expect_eq } from "cap:test";
import {
    CSS_COLOR_COUNT,
    RgbColor,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
    isNamedColor,
} from "../../packages/color-name/src/index.cp";

fn main() {
    test("1. @cap/color-name — CSS color count & case-insensitive lookup", () => {
        expect_eq(CSS_COLOR_COUNT, 148 as usize);
        const c1: RgbColor = lookupColor("cornflowerblue");
        expect_eq(c1.found, true);
        expect_eq(c1.r, 100 as u8);
        expect_eq(c1.g, 149 as u8);
        expect_eq(c1.b, 237 as u8);

        const c2: RgbColor = lookupColor("REBECCAPURPLE");
        expect_eq(c2.found, true);
        expect_eq(c2.r, 102 as u8);
        expect_eq(c2.g, 51 as u8);
        expect_eq(c2.b, 153 as u8);

        expect_eq(lookupColor("notacolor").found, false);
    });

    test("2. @cap/color-name — lookupColorHex, reverseLookupColor & isNamedColor", () => {
        expect_eq(lookupColorHex("red"), 0xFF0000 as i32);
        expect_eq(lookupColorHex("white"), 0xFFFFFF as i32);
        expect_eq(lookupColorHex("unknown"), -1 as i32);

        expect_eq(reverseLookupColor(255 as u8, 0 as u8, 0 as u8), "red");
        expect_eq(reverseLookupColor(102 as u8, 51 as u8, 153 as u8), "rebeccapurple");
        expect_eq(reverseLookupColor(1 as u8, 2 as u8, 3 as u8), "");

        expect_eq(isNamedColor("tomato"), true);
        expect_eq(isNamedColor("invalid"), false);
    });
}
