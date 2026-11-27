// cap/test/testCapHumanizeString.cp
// Comprehensive unit tests for @cap/humanize-string (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    humanizeString,
} from "../../packages/humanize-string/src/index.cp";

fn main() {
    test("1. @cap/humanize-string — converts camelCase and PascalCase via @cap/decamelize", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeString("fooBar", buf[0..128]);
        expect_eq(buf[0..n1], "Foo bar");

        const n2 = humanizeString("UnicornsAndRainbows", buf[0..128]);
        expect_eq(buf[0..n2], "Unicorns and rainbows");
    });

    test("2. @cap/humanize-string — converts mixed camelCase, kebab-case, and snake_case", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeString("fooBar-baz_qux", buf[0..128]);
        expect_eq(buf[0..n1], "Foo bar baz qux");

        const n2 = humanizeString("foo-bar_baz", buf[0..128]);
        expect_eq(buf[0..n2], "Foo bar baz");
    });

    test("3. @cap/humanize-string — trims leading/trailing whitespace and separators", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeString("  hello_world  ", buf[0..128]);
        expect_eq(buf[0..n1], "Hello world");

        const n2 = humanizeString("__--leadingAndTrailing--__", buf[0..128]);
        expect_eq(buf[0..n2], "Leading and trailing");
    });

    test("4. @cap/humanize-string — collapses consecutive underscores, hyphens, and whitespace", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeString("foo___bar---baz   \t\n qux", buf[0..128]);
        expect_eq(buf[0..n1], "Foo bar baz qux");

        const n2 = humanizeString("already humanized", buf[0..128]);
        expect_eq(buf[0..n2], "Already humanized");
    });

    test("5. @cap/humanize-string — handles single words, acronyms, and digits", () => {
        let buf: [128]u8 = undefined;
        const n1 = humanizeString("unicorn", buf[0..128]);
        expect_eq(buf[0..n1], "Unicorn");

        const n2 = humanizeString("parseHTML5Doc", buf[0..128]);
        expect_eq(buf[0..n2], "Parse html5 doc");
    });

    test("6. @cap/humanize-string — empty/separator-only inputs and bounded output buffer", () => {
        let buf: [64]u8 = undefined;
        expect_eq(humanizeString("", buf[0..64]), 0 as usize);
        expect_eq(humanizeString("   ---___\t\n  ", buf[0..64]), 0 as usize);

        let tiny: [7]u8 = undefined;
        const nTiny = humanizeString("fooBarBaz", tiny[0..7]);
        expect_eq(nTiny, 7 as usize);
        expect_eq(tiny[0..nTiny], "Foo bar");
    });
}
