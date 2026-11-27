// cap/test/testCapUserAgentHints.cp
// Comprehensive unit tests for @cap/user-agent-hints

import { test, expect_eq } from "cap:test";
import {
    BrandVersion,
    parseBrands,
    isGreaseBrand,
    findBrandVersion,
    parseMobile,
    parsePlatform,
    formatBrands,
} from "../../packages/sfv/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn main() {
    test("1. @cap/user-agent-hints — Parse multi-brand Chrome/Edge Sec-CH-UA list", () => {
        const hdr = "\"Chromium\";v=\"124\", \"Google Chrome\";v=\"124.0.6367.91\", \"Not-A.Brand\";v=\"99\"";
        let out: [4]BrandVersion = undefined;
        const n = parseBrands(hdr, out[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(sliceEq(out[0].brand, "Chromium"), true);
        expect_eq(sliceEq(out[0].version, "124"), true);
        expect_eq(sliceEq(out[1].brand, "Google Chrome"), true);
        expect_eq(sliceEq(out[1].version, "124.0.6367.91"), true);
        expect_eq(sliceEq(out[2].brand, "Not-A.Brand"), true);
        expect_eq(sliceEq(out[2].version, "99"), true);
    });

    test("2. @cap/user-agent-hints — GREASE brand detection & quoted comma/semicolon handling", () => {
        expect_eq(isGreaseBrand("Not-A.Brand"), true);
        expect_eq(isGreaseBrand("Not/A)Brand"), true);
        expect_eq(isGreaseBrand("Not A;Brand"), true);
        expect_eq(isGreaseBrand("Chromium"), false);
        expect_eq(isGreaseBrand("Microsoft Edge"), false);

        const tricky = "\"Not,A;Brand\";v=\"8\", \"Microsoft Edge\";v=\"125\"";
        let out: [4]BrandVersion = undefined;
        const n = parseBrands(tricky, out[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(isGreaseBrand(out[0].brand), true);
        expect_eq(sliceEq(out[1].brand, "Microsoft Edge"), true);
        expect_eq(sliceEq(out[1].version, "125"), true);
    });

    test("3. @cap/user-agent-hints — Case-insensitive findBrandVersion lookup", () => {
        const hdr = "\"Not:A-Brand\";v=\"99\", \"Chromium\";v=\"126\", \"Brave\";v=\"126\"";
        expect_eq(sliceEq(findBrandVersion(hdr, "chromium"), "126"), true);
        expect_eq(sliceEq(findBrandVersion(hdr, "BRAVE"), "126"), true);
        expect_eq(findBrandVersion(hdr, "Safari").len, 0 as usize);
    });

    test("4. @cap/user-agent-hints — parseMobile (?1 / ?0 / invalid)", () => {
        expect_eq(parseMobile("?1"), 1 as i32);
        expect_eq(parseMobile("  ?0  "), 0 as i32);
        expect_eq(parseMobile("?2"), -1 as i32);
        expect_eq(parseMobile("true"), -1 as i32);
        expect_eq(parseMobile(""), -1 as i32);
    });

    test("5. @cap/user-agent-hints — parsePlatform quote stripping & validation", () => {
        expect_eq(sliceEq(parsePlatform("\"macOS\""), "macOS"), true);
        expect_eq(sliceEq(parsePlatform(" \"Windows\" "), "Windows"), true);
        expect_eq(sliceEq(parsePlatform("\"Linux\""), "Linux"), true);
        expect_eq(parsePlatform("macOS").len, 0 as usize);
        expect_eq(parsePlatform("\"unterminated").len, 0 as usize);
    });

    test("6. @cap/user-agent-hints — formatBrands round-trip & small buffer protection", () => {
        let brands: [2]BrandVersion = undefined;
        brands[0] = BrandVersion{ brand: "Chromium", version: "124" };
        brands[1] = BrandVersion{ brand: "Google Chrome", version: "124" };

        let buf: [128]u8 = undefined;
        const n = formatBrands(brands[0..2], buf[0..128]);
        const expected = "\"Chromium\";v=\"124\", \"Google Chrome\";v=\"124\"";
        expect_eq(sliceEq(buf[0..n], expected), true);

        let parsed: [4]BrandVersion = undefined;
        expect_eq(parseBrands(buf[0..n], parsed[0..4]), 2 as usize);
        expect_eq(sliceEq(parsed[1].brand, "Google Chrome"), true);

        // Buffer only fits first entry ("\"Chromium\";v=\"124\"" = 18 bytes).
        let tiny: [24]u8 = undefined;
        const nt = formatBrands(brands[0..2], tiny[0..24]);
        expect_eq(sliceEq(tiny[0..nt], "\"Chromium\";v=\"124\""), true);
    });
}
