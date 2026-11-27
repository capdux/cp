import { test, expect_eq } from "cap:test";
import {
    DecamelizeOptions,
    decamelizedLen,
    decamelize,
    decamelizeDefault,
    decamelizeWithOptions,
    decamelizeOpts,
    toSnakeCase,
    toKebabCase,
    toSpaceCase,
    toConstantCase,
} from "../../packages/decamelize/src/index.cp";

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
    test("1. decamelize — Standard camelCase and PascalCase", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelize("unicornRainbow", "_", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "unicorn_rainbow"), true);
        expect_eq(n1, decamelizedLen("unicornRainbow", "_", false));

        let n2 = decamelize("UnicornRainbow", "-", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "unicorn-rainbow"), true);
    });

    test("2. decamelize — Handles acronyms cleanly", () => {
        let buf: [64]u8 = undefined;
        let n1 = toSnakeCase("parseHTTPResponse", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "parse_http_response"), true);

        let n2 = toKebabCase("XMLHttpRequest", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "xml-http-request"), true);
    });

    test("3. decamelize — preserveConsecutiveUppercase option", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelizeWithOptions("testURL", "_", true, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "test_URL"), true);

        let n2 = decamelizeWithOptions("myHTTPServer", "-", true, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "my-HTTP-server"), true);
    });

    test("4. decamelize — Digits followed by uppercase", () => {
        let buf: [64]u8 = undefined;
        let n1 = toSnakeCase("base64Encode", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "base64_encode"), true);

        let n2 = toKebabCase("sha256Hash", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "sha256-hash"), true);
    });

    test("5. decamelize — Multi-character custom separator", () => {
        let buf: [64]u8 = undefined;
        let n1 = decamelize("fooBarBaz", " :: ", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "foo :: bar :: baz"), true);
        expect_eq(n1, decamelizedLen("fooBarBaz", " :: ", false));
    });

    test("6. decamelize — Empty, already lowercase, and short buffer safety", () => {
        let buf: [8]u8 = undefined;
        expect_eq(toSnakeCase("", buf[0..8]), 0 as usize);
        let n1 = toSnakeCase("already_snake", buf[0..8]);
        expect_eq(n1, 8 as usize);
        expect_eq(eqBytes(buf[0..8], n1, "already_"), true);
    });

    test("7. decamelize — DecamelizeOptions struct & decamelizeOpts", () => {
        let buf: [64]u8 = undefined;
        const def = DecamelizeOptions.defaults();
        expect_eq(def.separator, "_");
        expect_eq(def.preserve_consecutive_uppercase, false);

        const n1 = decamelizeOpts("testURL", def, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "test_url"), true);

        const custom = DecamelizeOptions {
            separator: "-",
            preserve_consecutive_uppercase: true,
        };
        const n2 = decamelizeOpts("parseJSONData", custom, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "parse-JSON-data"), true);
    });

    test("8. decamelize — decamelizeDefault, toSpaceCase, and toConstantCase", () => {
        let buf: [64]u8 = undefined;
        const n1 = decamelizeDefault("helloWorld", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "hello_world"), true);

        const n2 = toSpaceCase("helloWorldAPI", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2, "hello world api"), true);

        const n3 = toConstantCase("maxRetryCount", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n3, "MAX_RETRY_COUNT"), true);
    });
}
