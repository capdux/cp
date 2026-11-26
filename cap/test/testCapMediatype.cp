import { test, expect_eq } from "cap:test";
import { ParsedMediaType, essenceEq } from "../../packages/@cap/mediatype/src/index.cp";

fn main() {
    test("1. @cap/mediatype — parses type/subtype+suffix and parameters", () => {
        let mt = ParsedMediaType.parse("Application/vnd.api+json; Charset=\"utf-8\"; version=2");
        expect_eq(mt.valid, true);
        expect_eq(mt.top_level, "Application");
        expect_eq(mt.sub_level, "vnd.api");
        expect_eq(mt.suffix, "json");
        expect_eq(mt.param_count, 2);
        expect_eq(mt.getParam("charset") ?? "", "utf-8");
        expect_eq(mt.getParam("VERSION") ?? "", "2");
    });

    test("2. @cap/mediatype — isJson() matches application/json and +json suffix", () => {
        let j1 = ParsedMediaType.parse("application/json");
        let j2 = ParsedMediaType.parse("application/problem+json; charset=utf-8");
        let t1 = ParsedMediaType.parse("text/plain");
        expect_eq(j1.isJson(), true);
        expect_eq(j2.isJson(), true);
        expect_eq(j2.hasSuffix("JSON"), true);
        expect_eq(t1.isJson(), false);
        expect_eq(t1.isText(), true);
    });

    test("3. @cap/mediatype — formatEssence & formatFull canonical lowercase output", () => {
        let mt = ParsedMediaType.parse("TEXT/HTML; Charset=utf-8");
        let b1: [128]u8 = [0; 128];
        let n1 = mt.formatEssence(b1[0..128]);
        expect_eq(b1[0..n1] as string, "text/html");

        let b2: [128]u8 = [0; 128];
        let n2 = mt.formatFull(b2[0..128]);
        expect_eq(b2[0..n2] as string, "text/html; charset=utf-8");
    });

    test("4. @cap/mediatype — essenceEq ignores parameters & case", () => {
        expect_eq(essenceEq("application/json; charset=utf-8", "APPLICATION/JSON"), true);
        expect_eq(essenceEq("text/html", "text/plain"), false);
    });

    test("5. @cap/mediatype — quoted parameter containing semicolon", () => {
        let mt = ParsedMediaType.parse("multipart/form-data; boundary=\"a;b;c\"");
        expect_eq(mt.valid, true);
        expect_eq(mt.param_count, 1);
        expect_eq(mt.getParam("boundary") ?? "", "a;b;c");
    });

    test("6. @cap/mediatype — rejects invalid media types & malformed params", () => {
        expect_eq(ParsedMediaType.parse("invalid").valid, false);
        expect_eq(ParsedMediaType.parse("text/plain; brokenparam").valid, false);
        expect_eq(essenceEq("bad", "text/plain"), false);
    });
}
