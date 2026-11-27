// cap/test/testCapCamelcase.cp
// Comprehensive unit tests for ident_case (Rust ident_case::RenameRule all 8 rules)

import { test, expect_eq } from "cap:test";
import {
    RenameRule,
    toLowerCase,
    toUpperCase,
    toCamelCase,
    toPascalCase,
    toSnakeCase,
    toKebabCase,
    toScreamingSnakeCase,
    toScreamingKebabCase,
    renameIdent,
    parseRenameRule,
} from "../../packages/ident_case/src/index.cp";

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
    test("1. ident_case — toCamelCase with multi-separators & leading/trailing symbols", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toCamelCase("--foo-bar_baz.qux--", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "fooBarBazQux"), true);

        const n2: usize = toCamelCase("XMLHttpRequest", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "xmlHttpRequest"), true);
    });

    test("2. ident_case — toPascalCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toPascalCase("foo-bar_baz", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "FooBarBaz"), true);

        const n2: usize = toPascalCase("hello world", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "HelloWorld"), true);
    });

    test("3. ident_case — toSnakeCase with acronyms XMLHttpRequest & getHTTPResponseCode", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toSnakeCase("XMLHttpRequest", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "xml_http_request"), true);

        const n2: usize = toSnakeCase("getHTTPResponseCode", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "get_http_response_code"), true);

        const n3: usize = toSnakeCase("fooBar", out[0..64]);
        expect_eq(sliceEq(out[0..n3], "foo_bar"), true);
    });

    test("4. ident_case — toKebabCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toKebabCase("fooBarBaz", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "foo-bar-baz"), true);

        const n2: usize = toKebabCase("HTTP_SERVER_PORT", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "http-server-port"), true);
    });

    test("5. ident_case — toScreamingSnakeCase & toScreamingKebabCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toScreamingSnakeCase("httpServerPort", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "HTTP_SERVER_PORT"), true);

        const n2: usize = toScreamingSnakeCase("max-retry.count", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "MAX_RETRY_COUNT"), true);

        const n3: usize = toScreamingKebabCase("httpServerPort", out[0..64]);
        expect_eq(sliceEq(out[0..n3], "HTTP-SERVER-PORT"), true);
    });

    test("6. ident_case — toLowerCase & toUpperCase", () => {
        let out: [64]u8 = undefined;
        const n1: usize = toLowerCase("Foo_Bar-Baz", out[0..64]);
        expect_eq(sliceEq(out[0..n1], "foobarbaz"), true);

        const n2: usize = toUpperCase("foo_bar-baz", out[0..64]);
        expect_eq(sliceEq(out[0..n2], "FOOBARBAZ"), true);
    });

    test("7. ident_case — renameIdent dispatches all 8 RenameRule variants", () => {
        let out: [64]u8 = undefined;
        const src = "hello_world_v2";
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.LowerCase, src, out[0..64])], "helloworldv2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.UpperCase, src, out[0..64])], "HELLOWORLDV2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.CamelCase, src, out[0..64])], "helloWorldV2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.PascalCase, src, out[0..64])], "HelloWorldV2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.SnakeCase, src, out[0..64])], "hello_world_v2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.ScreamingSnakeCase, src, out[0..64])], "HELLO_WORLD_V2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.KebabCase, src, out[0..64])], "hello-world-v2"), true);
        expect_eq(sliceEq(out[0..renameIdent(RenameRule.ScreamingKebabCase, src, out[0..64])], "HELLO-WORLD-V2"), true);
    });

    test("8. ident_case — parseRenameRule parses all 8 canonical names & rejects unknown", () => {
        expect_eq(parseRenameRule("lowercase").? == RenameRule.LowerCase, true);
        expect_eq(parseRenameRule("UPPERCASE").? == RenameRule.UpperCase, true);
        expect_eq(parseRenameRule("camelCase").? == RenameRule.CamelCase, true);
        expect_eq(parseRenameRule("PascalCase").? == RenameRule.PascalCase, true);
        expect_eq(parseRenameRule("snake_case").? == RenameRule.SnakeCase, true);
        expect_eq(parseRenameRule("SCREAMING_SNAKE_CASE").? == RenameRule.ScreamingSnakeCase, true);
        expect_eq(parseRenameRule("kebab-case").? == RenameRule.KebabCase, true);
        expect_eq(parseRenameRule("SCREAMING-KEBAB-CASE").? == RenameRule.ScreamingKebabCase, true);
        expect_eq(parseRenameRule("unknown") == null, true);
    });

    test("9. ident_case — digit boundaries, empty input & tiny buffer protection", () => {
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
