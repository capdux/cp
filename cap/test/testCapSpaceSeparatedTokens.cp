import { test, expect_eq } from "cap:test";
import {
    parseSpaceTokens,
    hasSpaceToken,
    stringifySpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
} from "../../packages/@cap/space-separated-tokens/src/index.cp";

fn main() {
    test("1. @cap/space-separated-tokens — parses whitespace-delimited tokens", () => {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = parseSpaceTokens("  btn\tbtn-primary\n active  ", toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0], "btn");
        expect_eq(toks[1], "btn-primary");
        expect_eq(toks[2], "active");
    });

    test("2. @cap/space-separated-tokens — hasSpaceToken matches exact tokens only", () => {
        expect_eq(hasSpaceToken("foo bar-baz qux", "foo"), true);
        expect_eq(hasSpaceToken("foo bar-baz qux", "bar"), false);
        expect_eq(hasSpaceToken("foo bar-baz qux", "bar-baz"), true);
        expect_eq(hasSpaceToken("", "foo"), false);
    });

    test("3. @cap/space-separated-tokens — stringifySpaceTokens skips blanks and joins with single space", () => {
        const items: [4]string = [" alpha ", "", "beta", "  gamma\t"];
        let buf: [64]u8 = [0; 64];
        const n = stringifySpaceTokens(items[0..4], buf[0..64]);
        expect_eq(buf[0..n] as string, "alpha beta gamma");
    });

    test("4. @cap/space-separated-tokens — parseCommaTokens trims and ignores trailing comma", () => {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = parseCommaTokens(" text/html , application/json, */* , ", toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0], "text/html");
        expect_eq(toks[1], "application/json");
        expect_eq(toks[2], "*/*");
    });

    test("5. @cap/space-separated-tokens — stringifyCommaTokens formats HTML comma list", () => {
        const items: [3]string = ["a", "b", "c"];
        let buf: [64]u8 = [0; 64];
        const n = stringifyCommaTokens(items[0..3], buf[0..64]);
        expect_eq(buf[0..n] as string, "a, b, c");
    });

    test("6. @cap/space-separated-tokens — handles empty and whitespace-only inputs", () => {
        let toks: [4]string = ["", "", "", ""];
        expect_eq(parseSpaceTokens("   \t\n ", toks[0..4]), 0 as usize);
        expect_eq(parseCommaTokens("   ", toks[0..4]), 0 as usize);
    });
}
