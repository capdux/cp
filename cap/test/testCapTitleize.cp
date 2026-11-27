// cap/test/testCapTitleize.cp
// Comprehensive unit tests for @cap/titleize (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    titleize,
    upperCaseFirst,
} from "../../packages/titleize/src/index.cp";

fn main() {
    test("1. @cap/titleize — capitalizes words separated by spaces", () => {
        let buf: [128]u8 = undefined;
        const n1 = titleize("unicorns and rainbows", buf[0..128]);
        expect_eq(buf[0..n1], "Unicorns And Rainbows");

        const n2 = titleize("UNICORNS AND RAINBOWS", buf[0..128]);
        expect_eq(buf[0..n2], "Unicorns And Rainbows");

        const n3 = titleize("  hello   world  ", buf[0..128]);
        expect_eq(buf[0..n3], "  Hello   World  ");
    });

    test("2. @cap/titleize — handles hyphens, underscores, slashes, and punctuation", () => {
        let buf: [128]u8 = undefined;
        const n1 = titleize("FOO-BAR", buf[0..128]);
        expect_eq(buf[0..n1], "Foo-Bar");

        const n2 = titleize("foo_bar/baz.qux", buf[0..128]);
        expect_eq(buf[0..n2], "Foo_Bar/Baz.Qux");

        const n3 = titleize("(hello) \"WORLD\"!", buf[0..128]);
        expect_eq(buf[0..n3], "(Hello) \"World\"!");
    });

    test("3. @cap/titleize — preserves apostrophes inside contractions and mixed digits", () => {
        let buf: [128]u8 = undefined;
        const n1 = titleize("don't stop believing", buf[0..128]);
        expect_eq(buf[0..n1], "Don't Stop Believing");

        const n2 = titleize("v2 release 100% READY", buf[0..128]);
        expect_eq(buf[0..n2], "V2 Release 100% Ready");
    });

    test("4. @cap/titleize — upperCaseFirst only uppercases first character", () => {
        let buf: [128]u8 = undefined;
        const n1 = upperCaseFirst("helloWorld", buf[0..128]);
        expect_eq(buf[0..n1], "HelloWorld");

        const n2 = upperCaseFirst("AlreadyUpper", buf[0..128]);
        expect_eq(buf[0..n2], "AlreadyUpper");

        const n3 = upperCaseFirst("123abc", buf[0..128]);
        expect_eq(buf[0..n3], "123abc");
    });

    test("5. @cap/titleize — single-character and tab/newline whitespace inputs", () => {
        let buf: [128]u8 = undefined;
        const n1 = titleize("a", buf[0..128]);
        expect_eq(buf[0..n1], "A");

        const n2 = titleize("first\tsecond\nthird", buf[0..128]);
        expect_eq(buf[0..n2], "First\tSecond\nThird");

        const n3 = upperCaseFirst("z", buf[0..128]);
        expect_eq(buf[0..n3], "Z");
    });

    test("6. @cap/titleize — empty input and bounded output buffer", () => {
        let buf: [64]u8 = undefined;
        expect_eq(titleize("", buf[0..64]), 0 as usize);
        expect_eq(upperCaseFirst("", buf[0..64]), 0 as usize);

        let tiny: [5]u8 = undefined;
        const n1 = titleize("hello world", tiny[0..5]);
        expect_eq(n1, 5 as usize);
        expect_eq(tiny[0..n1], "Hello");

        const n2 = upperCaseFirst("abcdef", tiny[0..5]);
        expect_eq(n2, 5 as usize);
        expect_eq(tiny[0..n2], "Abcde");
    });
}
