// cap/test/testCapCamelcase.cp
// Comprehensive unit tests for @cap/camelcase (camelcase / change-case / heck parity)

import { test, expect_eq } from "cap:test";
import {
    toCamelCase,
    toPascalCase,
    toSnakeCase,
    toKebabCase,
    toScreamingSnakeCase,
} from "../../packages/@cap/camelcase/src/index.cp";

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
    test("1. @cap/camelcase — toCamelCase with multi-separators & leading/trailing symbols", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toCamelCase("--foo-bar_baz.qux--", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "fooBarBazQux"), true);

        const n2: usize = toCamelCase("XMLHttpRequest", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "xmlHttpRequest"), true);
    });

    test("2. @cap/camelcase — toPascalCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toPascalCase("foo-bar_baz", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "FooBarBaz"), true);

        const n2: usize = toPascalCase("hello world", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "HelloWorld"), true);
    });

    test("3. @cap/camelcase — toSnakeCase with acronyms XMLHttpRequest & getHTTPResponseCode", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toSnakeCase("XMLHttpRequest", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "xml_http_request"), true);

        const n2: usize = toSnakeCase("getHTTPResponseCode", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "get_http_response_code"), true);

        const n3: usize = toSnakeCase("fooBar", out[0..64]);
        expect_eq(sliceEq(out[0..n3], "foo_bar"), true);
    });

    test("4. @cap/camelcase — toKebabCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toKebabCase("fooBarBaz", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "foo-bar-baz"), true);

        const n2: usize = toKebabCase("HTTP_SERVER_PORT", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "http-server-port"), true);
    });

    test("5. @cap/camelcase — toScreamingSnakeCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toScreamingSnakeCase("httpServerPort", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "HTTP_SERVER_PORT"), true);

        const n2: usize = toScreamingSnakeCase("max-retry.count", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "MAX_RETRY_COUNT"), true);
    });

    test("6. @cap/camelcase — digit boundaries, empty input & tiny buffer protection", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toSnakeCase("apiV2Client", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "api_v2_client"), true);

        expect_eq(toCamelCase("", out[0..64]), 0 as usize);
        expect_eq(toSnakeCase("---___---", out[0..64]), 0 as usize);

        let tiny: [4]u8 = undefined;
        expect_eq(toSnakeCase("helloWorld", tiny[0..4]), 0 as usize);
        expect_eq(toCamelCase("hello-world", tiny[0..4]), 0 as usize);
    });
}
