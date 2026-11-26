import { test, expect_eq } from "cap:test";
import {
    isSafeUri,
    sanitizeUri,
} from "../../packages/@cap/sanitize-uri/src/index.cp";

fn main() {
    test("1. @cap/sanitize-uri — allows safe http/https/mailto/tel and relative URIs", () => {
        expect_eq(isSafeUri("https://example.com/path?q=1#top"), true);
        expect_eq(isSafeUri("mailto:dev@example.com"), true);
        expect_eq(isSafeUri("/docs/getting-started.md"), true);
        expect_eq(isSafeUri("./foo:bar"), true);
        expect_eq(isSafeUri("#section-1"), true);
    });

    test("2. @cap/sanitize-uri — blocks unsafe script/data/file schemes", () => {
        expect_eq(isSafeUri("javascript:alert(1)"), false);
        expect_eq(isSafeUri("JAVASCRIPT:void(0)"), false);
        expect_eq(isSafeUri("vbscript:msgbox(1)"), false);
        expect_eq(isSafeUri("data:text/html,hello"), false);
        expect_eq(isSafeUri("file:///etc/passwd"), false);
    });

    test("3. @cap/sanitize-uri — blocks whitespace/control-char obfuscated schemes", () => {
        expect_eq(isSafeUri("  java\tscript:alert(1)"), false);
        expect_eq(isSafeUri("\njavascript:x"), false);
    });

    test("4. @cap/sanitize-uri — sanitizeUri percent-encodes spaces, quotes, and angle brackets", () => {
        let buf: [128]u8 = [0; 128];
        const n = sanitizeUri("https://example.com/a b/<tag>", buf[0..128]);
        expect_eq(buf[0..n] as string, "https://example.com/a%20b/%3Ctag%3E");
    });

    test("5. @cap/sanitize-uri — preserves valid %XX escapes and escapes lone '%'", () => {
        let buf: [128]u8 = [0; 128];
        const n = sanitizeUri("https://example.com/100%20ok/50%", buf[0..128]);
        expect_eq(buf[0..n] as string, "https://example.com/100%20ok/50%25");
    });

    test("6. @cap/sanitize-uri — returns 0 length for blocked schemes", () => {
        let buf: [128]u8 = [0; 128];
        expect_eq(sanitizeUri("javascript:alert(1)", buf[0..128]), 0 as usize);
        expect_eq(sanitizeUri("data:text/plain;base64,QQ==", buf[0..128]), 0 as usize);
    });
}
