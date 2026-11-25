// cap/test/testJsoncDeep.cp
// Deep unit tests for cap:jsonc (Rust `json_comments` parity: stripJsoncInPlace, stripJsoncWithSpaces,
// JsoncOptions, string-aware URL preservation, and malformed block-comment / string rejection).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JsoncOptions,
    stripJsoncWithOptions,
    stripJsoncWithSpaces,
    stripJsoncInPlace,
    stripJsonc,
    stripJsoncPreserveLines,
    hasJsoncComments,
    countJsoncTrailingCommas,
} from "cap:jsonc";

fn main() {
    test("1. cap:jsonc — Strip Comments, Trailing Commas & Preserve String URLs", () => {
        const raw = "{\n  // header\n  \"url\": \"https://cp-lang.org/*keep*/\",\n  \"ok\": true,\n}";
        expect_eq(hasJsoncComments(raw), true);
        expect_eq(countJsoncTrailingCommas(raw), 1 as usize);

        let buf: [256]u8 = [0; 256];
        const clean = stripJsonc(raw, buf[0..]).?;
        expect_eq(std.mem.indexOf(u8, clean, "https://cp-lang.org/*keep*/") != null, true);
        expect_eq(std.mem.indexOf(u8, clean, "header") == null, true);
        expect_eq(countJsoncTrailingCommas(clean), 0 as usize);
    });

    test("2. cap:jsonc — stripJsoncWithSpaces & stripJsoncInPlace 1:1 Byte Alignment", () => {
        const src = "{\"a\": /* hi\nthere */ 1,}";
        let out: [64]u8 = [0; 64];
        const spaced = stripJsoncWithSpaces(src, out[0..]).?;
        expect_eq(spaced.len, src.len);
        // Byte offset of '1' is preserved identically
        expect_eq(spaced[21], '1');
        // Trailing comma replaced by space
        expect_eq(spaced[22], ' ');
        expect_eq(spaced[23], '}');

        let inplace: [64]u8 = [0; 64];
        @memcpy(inplace[0..src.len], src);
        const ip = stripJsoncInPlace(inplace[0..src.len]).?;
        expect_eq(std.mem.eql(u8, ip, spaced), true);
    });

    test("3. cap:jsonc — Selective JsoncOptions & Malformed Input Rejection", () => {
        const src = "{\"x\": 1, /* c */}";
        let out: [64]u8 = [0; 64];
        const keep_comma = stripJsoncWithOptions(
            src,
            JsoncOptions{ strip_comments: true, strip_trailing_commas: false },
            out[0..],
        ).?;
        expect_eq(std.mem.eql(u8, keep_comma, "{\"x\": 1, }"), true);

        // Unterminated block comment or string returns null
        expect_eq(stripJsonc("{\"a\": /* unclosed", out[0..]) == null, true);
        expect_eq(stripJsonc("{\"a\": \"unclosed", out[0..]) == null, true);
    });
}
