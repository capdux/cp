import { test, expect_eq } from "cap:test";
import {
    parseSpaceTokens,
    parse,
    countTokens,
    tokenAt,
    hasSpaceToken,
    containsToken,
    stringifySpaceTokens,
    stringify,
    normalizeSpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
} from "../../packages/space-separated-tokens/src/index.cp";

fn main() {
    test("1. space-separated-tokens — parses whitespace-delimited tokens", () => {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = parseSpaceTokens("  btn\tbtn-primary\n active  ", toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0], "btn");
        expect_eq(toks[1], "btn-primary");
        expect_eq(toks[2], "active");

        const n2 = parse("\r\n  alpha   beta\t\tgamma \n", toks[0..8]);
        expect_eq(n2, 3 as usize);
        expect_eq(toks[0], "alpha");
        expect_eq(toks[1], "beta");
        expect_eq(toks[2], "gamma");
    });

    test("2. space-separated-tokens — hasSpaceToken & containsToken match exact tokens only", () => {
        expect_eq(hasSpaceToken("foo bar-baz qux", "foo"), true);
        expect_eq(hasSpaceToken("foo bar-baz qux", "bar"), false);
        expect_eq(containsToken("foo bar-baz qux", "bar-baz"), true);
        expect_eq(containsToken("foo bar-baz qux", ""), false);
        expect_eq(containsToken("", "foo"), false);
    });

    test("3. space-separated-tokens — stringifySpaceTokens & stringify skip blanks", () => {
        const items: [4]string = [" alpha ", "", "beta", "  gamma\t"];
        let buf: [64]u8 = undefined;
        const n = stringifySpaceTokens(items[0..4], buf[0..64]);
        expect_eq(buf[0..n] as string, "alpha beta gamma");

        const n2 = stringify(items[0..4], buf[0..64]);
        expect_eq(buf[0..n2] as string, "alpha beta gamma");
    });

    test("4. space-separated-tokens — countTokens & tokenAt zero-alloc indexing", () => {
        const src = "  first\t\tsecond\nthird   fourth  ";
        expect_eq(countTokens(src), 4 as usize);
        expect_eq(countTokens("   \t\r\n "), 0 as usize);
        expect_eq(countTokens(""), 0 as usize);

        expect_eq(tokenAt(src, 0) ?? "", "first");
        expect_eq(tokenAt(src, 1) ?? "", "second");
        expect_eq(tokenAt(src, 2) ?? "", "third");
        expect_eq(tokenAt(src, 3) ?? "", "fourth");
        expect_eq(tokenAt(src, 4) == null, true);
    });

    test("5. space-separated-tokens — normalizeSpaceTokens collapses whitespace", () => {
        let buf: [64]u8 = undefined;
        const n = normalizeSpaceTokens("  \tfoo   bar\r\n\tbaz  ", buf[0..64]);
        expect_eq(buf[0..n] as string, "foo bar baz");

        expect_eq(normalizeSpaceTokens("   \t\n", buf[0..64]), 0 as usize);
    });

    test("6. space-separated-tokens — parseCommaTokens trims and ignores trailing comma", () => {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = parseCommaTokens(" text/html , application/json, */* , ", toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0], "text/html");
        expect_eq(toks[1], "application/json");
        expect_eq(toks[2], "*/*");
    });

    test("7. space-separated-tokens — stringifyCommaTokens formats HTML comma list", () => {
        const items: [3]string = ["a", "b", "c"];
        let buf: [64]u8 = undefined;
        const n = stringifyCommaTokens(items[0..3], buf[0..64]);
        expect_eq(buf[0..n] as string, "a, b, c");
    });

    test("8. space-separated-tokens — handles empty, whitespace-only & output truncation", () => {
        let toks: [2]string = ["", ""];
        expect_eq(parseSpaceTokens("   \t\n ", toks[0..2]), 0 as usize);
        expect_eq(parseCommaTokens("   ", toks[0..2]), 0 as usize);
        // Output slice smaller than token count safely truncates without overflow
        expect_eq(parse("a b c d", toks[0..2]), 2 as usize);
        expect_eq(toks[0], "a");
        expect_eq(toks[1], "b");
    });
}
