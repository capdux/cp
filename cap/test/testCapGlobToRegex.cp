import { test, expect_eq } from "cap:test";
import { GlobOptions, globToRegex, globToRegexDefault } from "../../packages/glob-to-regexp/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/glob-to-regex — Basic *.cp -> ^[^/]*\\.cp$", () => {
        let buf: [128]u8 = undefined;
        const n = globToRegexDefault("*.cp", buf[0..128]);
        const expected = "^[^/]*\\.cp$";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("2. @cap/glob-to-regex — Globstar src/**/*.cp cross-directory expansion", () => {
        let buf: [128]u8 = undefined;
        const n1 = globToRegexDefault("src/**/*.cp", buf[0..128]);
        const exp1 = "^src(?:/.+/|/)[^/]*\\.cp$";
        expect_eq(n1, exp1.len as i64);
        expect_eq(testEq(buf[0..n1 as usize], exp1), true);

        const n2 = globToRegexDefault("**/*.zig", buf[0..128]);
        const exp2 = "^(?:.+/)?[^/]*\\.zig$";
        expect_eq(n2, exp2.len as i64);
        expect_eq(testEq(buf[0..n2 as usize], exp2), true);
    });

    test("3. @cap/glob-to-regex — Extended char class [!0-9] -> [^0-9] & ? -> .", () => {
        let buf: [128]u8 = undefined;
        const n = globToRegexDefault("file?.[!0-9]", buf[0..128]);
        const expected = "^file.\\.[^0-9]$";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);

        // With extended=false, ? and [ are escaped literally
        const n_plain = globToRegex("a?[0]", GlobOptions { extended: false, globstar: false, anchored: true }, buf[0..128]);
        const exp_plain = "^a\\?\\[0\\]$";
        expect_eq(testEq(buf[0..n_plain as usize], exp_plain), true);
    });

    test("4. @cap/glob-to-regex — Brace alternation {ts,cp,zig} -> (ts|cp|zig)", () => {
        let buf: [128]u8 = undefined;
        const n = globToRegexDefault("src/*.{ts,cp,zig}", buf[0..128]);
        const expected = "^src/[^/]*\\.(ts|cp|zig)$";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("5. @cap/glob-to-regex — Special regex chars a+b(1)$c.txt escaped verbatim", () => {
        let buf: [128]u8 = undefined;
        const n = globToRegexDefault("a+b(1)$c.txt", buf[0..128]);
        const expected = "^a\\+b\\(1\\)\\$c\\.txt$";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);
    });

    test("6. @cap/glob-to-regex — anchored=false & undersized buffer returns -1", () => {
        let buf: [128]u8 = undefined;
        const n = globToRegex("*.cp", GlobOptions { extended: true, globstar: true, anchored: false }, buf[0..128]);
        const expected = "[^/]*\\.cp";
        expect_eq(n, expected.len as i64);
        expect_eq(testEq(buf[0..n as usize], expected), true);

        let tiny: [4]u8 = undefined;
        expect_eq(globToRegexDefault("src/**/*.cp", tiny[0..4]), -1 as i64);
    });
}
