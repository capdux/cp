// cap/test/testCapHeckCase.cp
// Unit tests for @cap/heck-case (Rust heck v0.5 compatible case conversions)

import { test, expect_eq } from "cap:test";
import {
    toUpperCamelCase,
    toLowerCamelCase,
    toSnakeCase,
    toShoutySnakeCase,
    toKebabCase,
    toShoutyKebabCase,
    toTitleCase,
    toTrainCase,
} from "../../packages/@cap/heck-case/src/index.cp";

fn bufEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/heck-case — all 8 case styles from mixed phrase", () => {
        const src = "hello_world-example";
        let out: [64]u8 = undefined;

        const n1 = toUpperCamelCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n1], "HelloWorldExample"), true);

        const n2 = toLowerCamelCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n2], "helloWorldExample"), true);

        const n3 = toSnakeCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n3], "hello_world_example"), true);

        const n4 = toShoutySnakeCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n4], "HELLO_WORLD_EXAMPLE"), true);

        const n5 = toKebabCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n5], "hello-world-example"), true);

        const n6 = toShoutyKebabCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n6], "HELLO-WORLD-EXAMPLE"), true);

        const n7 = toTitleCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n7], "Hello World Example"), true);

        const n8 = toTrainCase(src, out[0..64]);
        expect_eq(bufEq(out[0..n8], "Hello-World-Example"), true);
    });

    test("2. @cap/heck-case — acronym splitting (XMLHttpRequest, HTTPSConnection, IOError)", () => {
        let out: [64]u8 = undefined;

        const n1 = toSnakeCase("XMLHttpRequest", out[0..64]);
        expect_eq(bufEq(out[0..n1], "xml_http_request"), true);

        const n2 = toUpperCamelCase("XMLHttpRequest", out[0..64]);
        expect_eq(bufEq(out[0..n2], "XmlHttpRequest"), true);

        const n3 = toLowerCamelCase("XMLHttpRequest", out[0..64]);
        expect_eq(bufEq(out[0..n3], "xmlHttpRequest"), true);

        const n4 = toKebabCase("HTTPSConnection", out[0..64]);
        expect_eq(bufEq(out[0..n4], "https-connection"), true);

        const n5 = toSnakeCase("IOError", out[0..64]);
        expect_eq(bufEq(out[0..n5], "io_error"), true);
    });

    test("3. @cap/heck-case — digits & alphanumeric boundaries (foo2Bar, HTML5Parser)", () => {
        let out: [64]u8 = undefined;

        const n1 = toSnakeCase("foo2Bar", out[0..64]);
        expect_eq(bufEq(out[0..n1], "foo2_bar"), true);

        const n2 = toUpperCamelCase("foo2_bar", out[0..64]);
        expect_eq(bufEq(out[0..n2], "Foo2Bar"), true);

        const n3 = toKebabCase("HTML5Parser", out[0..64]);
        expect_eq(bufEq(out[0..n3], "html5-parser"), true);

        const n4 = toLowerCamelCase("v2_api_client", out[0..64]);
        expect_eq(bufEq(out[0..n4], "v2ApiClient"), true);
    });

    test("4. @cap/heck-case — SHOUTY_CASE & multiple/leading/trailing delimiters", () => {
        let out: [64]u8 = undefined;

        const n1 = toLowerCamelCase("SHOUTY_SNAKE_CASE", out[0..64]);
        expect_eq(bufEq(out[0..n1], "shoutySnakeCase"), true);

        const n2 = toTrainCase("__leading---and   trailing__", out[0..64]);
        expect_eq(bufEq(out[0..n2], "Leading-And-Trailing"), true);

        const n3 = toTitleCase("foo.bar/baz::qux", out[0..64]);
        expect_eq(bufEq(out[0..n3], "Foo Bar Baz Qux"), true);
    });

    test("5. @cap/heck-case — Unicode Latin-1 case conversion", () => {
        let out: [64]u8 = undefined;

        // "café_crème" -> "CaféCrème"
        const n1 = toUpperCamelCase("caf\xC3\xA9_cr\xC3\xA8me", out[0..64]);
        expect_eq(bufEq(out[0..n1], "Caf\xC3\xA9Cr\xC3\xA8me"), true);

        // "CAFÉ_CRÈME" -> "café-crème"
        const n2 = toKebabCase("CAF\xC3\x89_CR\xC3\x88ME", out[0..64]);
        expect_eq(bufEq(out[0..n2], "caf\xC3\xA9-cr\xC3\xA8me"), true);
    });

    test("6. @cap/heck-case — empty input, separator-only input & small buffer truncation", () => {
        let out: [64]u8 = undefined;
        expect_eq(toSnakeCase("", out[0..64]), 0 as usize);
        expect_eq(toKebabCase("___---   ", out[0..64]), 0 as usize);

        let tiny: [5]u8 = undefined;
        const n1 = toSnakeCase("helloWorld", tiny[0..5]);
        expect_eq(n1, 5 as usize);
        expect_eq(bufEq(tiny[0..n1], "hello"), true);

        let zero: [0]u8 = undefined;
        expect_eq(toUpperCamelCase("hello", zero[0..0]), 0 as usize);
    });
}
