// cap/test/testCapProperties.cp
// Comprehensive unit tests for @cap/properties (Java .properties parity)

import { test, expect_eq } from "cap:test";
import { Properties, formatEntry } from "../../packages/@cap/properties/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/properties — '=' / ':' / space key-value separators", () => {
        const src = "k1 = alpha\nk2 : beta\nk3 gamma\n";
        const p: Properties = Properties.parse(src);
        expect_eq(p.len(), 3 as usize);
        expect_eq(sliceEq(p.get("k1"), "alpha"), true);
        expect_eq(sliceEq(p.get("k2"), "beta"), true);
        expect_eq(sliceEq(p.get("k3"), "gamma"), true);
    });

    test("2. @cap/properties — '#' and '!' comments & blank lines", () => {
        const src = "\n  # hash comment\n\t! bang comment\n\nserver.port = 8080\n";
        const p: Properties = Properties.parse(src);
        expect_eq(p.len(), 1 as usize);
        expect_eq(sliceEq(p.get("server.port"), "8080"), true);
    });

    test("3. @cap/properties — backslash multiline continuation", () => {
        const src = "fruits = apple, \\\n    banana, \\\r\n\tcherry\n";
        const p: Properties = Properties.parse(src);
        expect_eq(p.len(), 1 as usize);
        expect_eq(sliceEq(p.get("fruits"), "apple, banana, cherry"), true);
    });

    test("4. @cap/properties — \\n, \\t & \\u4E2D\\u6587 Unicode escape decoding to UTF-8", () => {
        const src = "msg = line1\\n\\tline2\nzh = \\u4E2D\\u6587\n";
        const p: Properties = Properties.parse(src);
        expect_eq(sliceEq(p.get("msg"), "line1\n\tline2"), true);
        expect_eq(sliceEq(p.get("zh"), "中文"), true);
    });

    test("5. @cap/properties — duplicate key last-wins & has/get miss", () => {
        const src = "env = dev\nenv = prod\n";
        const p: Properties = Properties.parse(src);
        expect_eq(p.has("env"), true);
        expect_eq(sliceEq(p.get("env"), "prod"), true);
        expect_eq(p.has("missing"), false);
        expect_eq(sliceEq(p.get("missing"), ""), true);
    });

    test("6. @cap/properties — formatEntry round-trip with escapes & tiny buffer returns -1", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = formatEntry("a=b:c d", "line1\nline2\\end", buf[0..128]);
        expect_eq(n > 0, true);

        const p: Properties = Properties.parse(buf[0..(n as usize)]);
        expect_eq(p.has("a=b:c d"), true);
        expect_eq(sliceEq(p.get("a=b:c d"), "line1\nline2\\end"), true);

        let tiny: [5]u8 = undefined;
        expect_eq(formatEntry("long.key", "value", tiny[0..5]), -1 as i64);
        expect_eq(formatEntry("", "value", buf[0..128]), -1 as i64);
    });
}
