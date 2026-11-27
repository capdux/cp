import { test, expect_eq } from "cap:test";
import {
    minIndent,
    stripIndent,
    indentLines,
} from "../../packages/dedent/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/indent-strip — minIndent ignoring blank & whitespace-only lines", () => {
        expect_eq(minIndent("    hello\n      world\n\n    !"), 4);
        expect_eq(minIndent("  \n\t\tfoo\n\tbar"), 1);
        expect_eq(minIndent("no indent\n  two"), 0);
        expect_eq(minIndent("   \n  \n"), 0);
    });

    test("2. @cap/indent-strip — stripIndent multiline block", () => {
        let src: string = "    fn main() {\n        print();\n    }\n";
        let buf: [128]u8 = undefined;
        let n: usize = stripIndent(src, buf[0..]);
        expect_eq(eqStr(buf[0..n], "fn main() {\n    print();\n}\n"), true);
    });

    test("3. @cap/indent-strip — stripIndent preserves blank lines & CRLF", () => {
        let src: string = "  a\r\n\r\n    b\r\n";
        let buf: [128]u8 = undefined;
        let n: usize = stripIndent(src, buf[0..]);
        expect_eq(eqStr(buf[0..n], "a\r\n\r\n  b\r\n"), true);
    });

    test("4. @cap/indent-strip — stripIndent with tabs", () => {
        let src: string = "\t\talpha\n\t\t\tbeta";
        let buf: [64]u8 = undefined;
        let n: usize = stripIndent(src, buf[0..]);
        expect_eq(eqStr(buf[0..n], "alpha\n\tbeta"), true);
    });

    test("5. @cap/indent-strip — indentLines skipping empty lines", () => {
        let src: string = "a\n\nb";
        let buf: [64]u8 = undefined;
        let n: usize = indentLines(src, 2, buf[0..]);
        expect_eq(eqStr(buf[0..n], "  a\n\n  b"), true);
    });

    test("6. @cap/indent-strip — empty input & small buffer protection", () => {
        let tiny: [3]u8 = undefined;
        expect_eq(stripIndent("", tiny[0..]), 0);
        expect_eq(stripIndent("    hello", tiny[0..]), 0);
        expect_eq(indentLines("hello", 2, tiny[0..]), 0);
    });
}
