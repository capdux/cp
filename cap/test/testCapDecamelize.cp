import { test, expect_eq } from "cap:test";
import { decamelize, decamelizeWithOptions, toSnakeCase, toKebabCase } from "../../packages/decamelize/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/decamelize — Standard camelCase and PascalCase", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelize("unicornRainbow", "_", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "unicorn_rainbow"), true);

        let n2 = decamelize("UnicornRainbow", "-", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "unicorn-rainbow"), true);
    });

    test("2. @cap/decamelize — Handles acronyms cleanly", () => {
        let buf: [64]u8 = undefined;
        let n1 = toSnakeCase("parseHTTPResponse", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "parse_http_response"), true);

        let n2 = toKebabCase("XMLHttpRequest", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "xml-http-request"), true);
    });

    test("3. @cap/decamelize — preserveConsecutiveUppercase option", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelizeWithOptions("testURL", "_", true, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "test_URL"), true);

        let n2 = decamelizeWithOptions("myHTTPServer", "-", true, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "my-HTTP-server"), true);
    });

    test("4. @cap/decamelize — Digits followed by uppercase", () => {
        let buf: [64]u8 = undefined;
        let n1 = toSnakeCase("base64Encode", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "base64_encode"), true);

        let n2 = toKebabCase("sha256Hash", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "sha256-hash"), true);
    });

    test("5. @cap/decamelize — Multi-character custom separator", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelize("fooBarBaz", " :: ", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "foo :: bar :: baz"), true);
    });

    test("6. @cap/decamelize — Empty, already lowercase, and short buffer safety", () => {
        let buf: [8]u8 = undefined;
        expect_eq(toSnakeCase("", buf[0..8]), 0);
        let n1 = toSnakeCase("already_snake", buf[0..8]);
        expect_eq(n1, 8);
        expect_eq(eqBytes(buf[0..8], n1, "already_"), true);
    });
}
