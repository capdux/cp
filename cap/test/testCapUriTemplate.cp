// cap/test/testCapUriTemplate.cp
// Comprehensive unit tests for @cap/uri-template

import { test, expect_eq } from "cap:test";
import {
    TemplateVar,
    isValid,
    countVars,
    expand,
    pctEncode,
    extractVarName,
} from "../../packages/url-template/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/uri-template — Level 1 simple & Level 2 reserved/fragment expansion", () => {
        let vars: [3]TemplateVar = undefined;
        vars[0] = TemplateVar{ name: "var", value: "hello world!" };
        vars[1] = TemplateVar{ name: "path", value: "/foo/bar" };
        vars[2] = TemplateVar{ name: "frag", value: "sec/1" };

        let buf1: [128]u8 = undefined;
        const n1 = expand("https://example.com/{var}", vars[0..3], buf1[0..128]);
        expect_eq(sliceEq(buf1[0..n1], "https://example.com/hello%20world%21"), true);

        let buf2: [128]u8 = undefined;
        const n2 = expand("https://example.com{+path}{#frag}", vars[0..3], buf2[0..128]);
        expect_eq(sliceEq(buf2[0..n2], "https://example.com/foo/bar#sec/1"), true);
    });

    test("2. @cap/uri-template — Level 3 query, continuation, path, label & semicolon params", () => {
        let vars: [4]TemplateVar = undefined;
        vars[0] = TemplateVar{ name: "x", value: "1024" };
        vars[1] = TemplateVar{ name: "y", value: "768" };
        vars[2] = TemplateVar{ name: "ext", value: "json" };
        vars[3] = TemplateVar{ name: "empty", value: "" };

        let b1: [128]u8 = undefined;
        const n1 = expand("/items{/x,y}{.ext}{?x,y}{&empty}", vars[0..4], b1[0..128]);
        expect_eq(sliceEq(b1[0..n1], "/items/1024/768.json?x=1024&y=768&empty="), true);

        let b2: [128]u8 = undefined;
        const n2 = expand("{;x,empty,y}", vars[0..4], b2[0..128]);
        expect_eq(sliceEq(b2[0..n2], ";x=1024;empty;y=768"), true);
    });

    test("3. @cap/uri-template — Level 4 prefix truncation {var:3}", () => {
        let vars: [2]TemplateVar = undefined;
        vars[0] = TemplateVar{ name: "var", value: "value" };
        vars[1] = TemplateVar{ name: "path", value: "/foo/bar" };

        let b1: [64]u8 = undefined;
        const n1 = expand("{var:3}", vars[0..2], b1[0..64]);
        expect_eq(sliceEq(b1[0..n1], "val"), true);

        let b2: [64]u8 = undefined;
        const n2 = expand("{+path:6}/here", vars[0..2], b2[0..64]);
        expect_eq(sliceEq(b2[0..n2], "/foo/b/here"), true);
    });

    test("4. @cap/uri-template — pctEncode reserved vs unreserved behavior", () => {
        let b1: [64]u8 = undefined;
        const n1 = pctEncode("a/b?c=1&d=2 ~_", false, b1[0..64]);
        expect_eq(sliceEq(b1[0..n1], "a%2Fb%3Fc%3D1%26d%3D2%20~_"), true);

        let b2: [64]u8 = undefined;
        const n2 = pctEncode("a/b?c=1%20end!", true, b2[0..64]);
        expect_eq(sliceEq(b2[0..n2], "a/b?c=1%20end!"), true);
    });

    test("5. @cap/uri-template — Missing variables are skipped cleanly", () => {
        let vars: [1]TemplateVar = undefined;
        vars[0] = TemplateVar{ name: "y", value: "ok" };

        let b1: [128]u8 = undefined;
        const n1 = expand("https://api.test/search{?missing1,y,missing2}", vars[0..1], b1[0..128]);
        expect_eq(sliceEq(b1[0..n1], "https://api.test/search?y=ok"), true);

        let b2: [128]u8 = undefined;
        const n2 = expand("https://api.test/only{?nope1,nope2}", vars[0..1], b2[0..128]);
        expect_eq(sliceEq(b2[0..n2], "https://api.test/only"), true);
    });

    test("6. @cap/uri-template — isValid, countVars & extractVarName", () => {
        const good = "/users/{id}/files{+path:6}{?page,limit}";
        expect_eq(isValid(good), true);
        expect_eq(countVars(good), 4 as usize);

        let nameBuf: [32]u8 = undefined;
        const l0 = extractVarName(good, 0, nameBuf[0..32]);
        expect_eq(sliceEq(nameBuf[0..l0], "id"), true);
        const l1 = extractVarName(good, 1, nameBuf[0..32]);
        expect_eq(sliceEq(nameBuf[0..l1], "path"), true);
        const l3 = extractVarName(good, 3, nameBuf[0..32]);
        expect_eq(sliceEq(nameBuf[0..l3], "limit"), true);
        expect_eq(extractVarName(good, 4, nameBuf[0..32]), 0 as usize);

        expect_eq(isValid("/bad/{unclosed"), false);
        expect_eq(isValid("/bad/extra}"), false);
        expect_eq(isValid("/bad/{}"), false);
        expect_eq(isValid("/bad/{var:0}"), false);
        expect_eq(isValid("/bad/{=reserved}"), false);
    });
}
