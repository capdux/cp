import { test, expect_eq } from "cap:test";
import {
    escapeUnsafeJson,
    serializeString,
    hasUnsafeScriptChars,
} from "../../packages/serialize-javascript/src/index.cp";

fn main() {
    test("1. @cap/serialize-javascript — escapes closing script tag breakout", () => {
        let buf: [128]u8 = [0; 128];
        const n = serializeString("</script><script>alert(1)</script>", buf[0..128]);
        expect_eq(
            buf[0..n] as string,
            "\"\\u003C\\u002Fscript\\u003E\\u003Cscript\\u003Ealert(1)\\u003C\\u002Fscript\\u003E\"",
        );
    });

    test("2. @cap/serialize-javascript — escapeUnsafeJson neutralizes raw JSON payloads", () => {
        let buf: [128]u8 = [0; 128];
        const n = escapeUnsafeJson("{\"html\":\"<b>hi</b>\"}", buf[0..128]);
        expect_eq(buf[0..n] as string, "{\"html\":\"\\u003Cb\\u003Ehi\\u003C\\u002Fb\\u003E\"}");
    });

    test("3. @cap/serialize-javascript — escapes Unicode line/paragraph separators U+2028 and U+2029", () => {
        const raw: [8]u8 = [0x61, 0xE2, 0x80, 0xA8, 0x62, 0xE2, 0x80, 0xA9];
        expect_eq(hasUnsafeScriptChars(raw[0..8] as string), true);
        let buf: [64]u8 = [0; 64];
        const n = serializeString(raw[0..8] as string, buf[0..64]);
        expect_eq(buf[0..n] as string, "\"a\\u2028b\\u2029\"");
    });

    test("4. @cap/serialize-javascript — escapes standard JSON quotes, backslashes, and control bytes", () => {
        let buf: [128]u8 = [0; 128];
        const n = serializeString("a\"b\\c\nd\te\x01", buf[0..128]);
        expect_eq(buf[0..n] as string, "\"a\\\"b\\\\c\\nd\\te\\u0001\"");
    });

    test("5. @cap/serialize-javascript — hasUnsafeScriptChars returns false for safe text", () => {
        expect_eq(hasUnsafeScriptChars("hello world 123 !@#"), false);
        expect_eq(hasUnsafeScriptChars("a < b"), true);
        expect_eq(hasUnsafeScriptChars("a/b"), true);
    });

    test("6. @cap/serialize-javascript — handles empty string cleanly", () => {
        let buf: [16]u8 = [0; 16];
        const n = serializeString("", buf[0..16]);
        expect_eq(buf[0..n] as string, "\"\"");
    });
}
