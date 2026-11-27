import { test, expect_eq } from "cap:test";
import {
    ContentTypeParam,
    ContentType,
    parseContentType,
    formatContentType,
    isValidMediaType,
    unescapeQuotedString,
} from "../../packages/content-type/src/index.cp";

fn main() {
    test("1. @cap/content-type — isValidMediaType validation", () => {
        expect_eq(isValidMediaType("text/html"), true);
        expect_eq(isValidMediaType("application/vnd.api+json"), true);
        expect_eq(isValidMediaType("multipart/form-data"), true);

        expect_eq(isValidMediaType(""), false);
        expect_eq(isValidMediaType("text"), false);
        expect_eq(isValidMediaType("text/"), false);
        expect_eq(isValidMediaType("/html"), false);
        expect_eq(isValidMediaType("text/html/extra"), false);
        expect_eq(isValidMediaType("text/html; charset=utf-8"), false);
        expect_eq(isValidMediaType("text/ht ml"), false);
    });

    test("2. @cap/content-type — Basic parsing & case-insensitive parameter lookup", () => {
        const ct = parseContentType("application/json; Charset=utf-8; Version=2");
        expect_eq(ct.valid, true);
        expect_eq(ct.media_type, "application/json");
        expect_eq(ct.param_count, 2);
        expect_eq(ct.charset() ?? "", "utf-8");
        expect_eq(ct.getParam("CHARSET") ?? "", "utf-8");
        expect_eq(ct.getParam("version") ?? "", "2");
        expect_eq(ct.boundary() == null, true);
    });

    test("3. @cap/content-type — Multipart boundary & quoted-string with backslash escapes", () => {
        const hdr = "multipart/form-data; boundary=\"----CapBoundary123\"; title=\"quoted \\\"val\\\" \\\\ ok\"";
        const ct = parseContentType(hdr);
        expect_eq(ct.valid, true);
        expect_eq(ct.media_type, "multipart/form-data");
        expect_eq(ct.param_count, 2);
        expect_eq(ct.boundary() ?? "", "----CapBoundary123");

        let buf: [64]u8 = undefined;
        const n = ct.getParamUnescaped("TITLE", buf[0..64]);
        expect_eq(buf[0..n], "quoted \"val\" \\ ok");

        let buf2: [32]u8 = undefined;
        const n2 = unescapeQuotedString("a\\\"b\\\\c", buf2[0..32]);
        expect_eq(buf2[0..n2], "a\"b\\c");
    });

    test("4. @cap/content-type — Strict RFC 7231 rejection of malformed headers", () => {
        expect_eq(parseContentType("").valid, false);
        expect_eq(parseContentType("text").valid, false);
        expect_eq(parseContentType("text/html;").valid, false);
        expect_eq(parseContentType("text/html; ; charset=utf-8").valid, false);
        expect_eq(parseContentType("text/html; charset=").valid, false);
        expect_eq(parseContentType("text/html; charset = utf-8").valid, false);
        expect_eq(parseContentType("text/html; charset=\"unclosed").valid, false);
        expect_eq(parseContentType("text/html charset=utf-8").valid, false);
        expect_eq(parseContentType("text/html; bad@key=val").valid, false);
    });

    test("5. @cap/content-type — formatContentType auto-quoting & escaping", () => {
        const params: [3]ContentTypeParam = [
            ContentTypeParam.new("charset", "utf-8"),
            ContentTypeParam.new("note", "hello world"),
            ContentTypeParam.new("quote", "say \"hi\" \\ bye"),
        ];
        let out: [128]u8 = undefined;
        const n = formatContentType("text/plain", params[0..3], out[0..128]);
        expect_eq(
            out[0..n],
            "text/plain; charset=utf-8; note=\"hello world\"; quote=\"say \\\"hi\\\" \\\\ bye\"",
        );

        // Round-trip parse the formatted output
        const rt = parseContentType(out[0..n]);
        expect_eq(rt.valid, true);
        expect_eq(rt.media_type, "text/plain");
        expect_eq(rt.charset() ?? "", "utf-8");
        expect_eq(rt.getParam("note") ?? "", "hello world");
        let unesc: [64]u8 = undefined;
        const un = rt.getParamUnescaped("quote", unesc[0..64]);
        expect_eq(unesc[0..un], "say \"hi\" \\ bye");
    });

    test("6. @cap/content-type — formatContentType rejects invalid media type or param names", () => {
        const ok_params: [1]ContentTypeParam = [ContentTypeParam.new("charset", "utf-8")];
        const bad_params: [1]ContentTypeParam = [ContentTypeParam.new("bad key", "val")];
        let out: [64]u8 = undefined;

        expect_eq(formatContentType("invalid", ok_params[0..1], out[0..64]), 0);
        expect_eq(formatContentType("text/html", bad_params[0..1], out[0..64]), 0);

        const n = formatContentType("application/octet-stream", ok_params[0..0], out[0..64]);
        expect_eq(out[0..n], "application/octet-stream");
    });
}
