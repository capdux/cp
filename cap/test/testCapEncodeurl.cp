import { test, expect_eq } from "cap:test";
import {
    needsUrlEncode,
    encodedUrlLen,
    encodeUrl,
    encodeUrlChecked,
    encodeUrlNormalized,
    decodeUrl,
    isEncodedUrl,
} from "../../packages/encodeurl/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/encodeurl — Preserves safe URL path, query & fragment characters", () => {
        let out: [128]u8 = undefined;
        let url = "https://capdux.org:443/api/v1/items?a=1&b=2#top";
        let n = encodeUrl(url, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, url), true);
        expect_eq(isEncodedUrl(url), true);
    });

    test("2. @cap/encodeurl — Encodes spaces, quotes, angle brackets & braces", () => {
        let out: [128]u8 = undefined;
        let n = encodeUrl("/hello world/<tag>\"{x}\"", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "/hello%20world/%3Ctag%3E%22%7Bx%7D%22"), true);
        expect_eq(isEncodedUrl("/hello world"), false);
    });

    test("3. @cap/encodeurl — Idempotent: preserves existing %XX without double-encoding", () => {
        let out: [128]u8 = undefined;
        let already = "/search?q=hello%20world&tag=%23cp";
        let n = encodeUrl(already, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, already), true);
        expect_eq(isEncodedUrl(already), true);
    });

    test("4. @cap/encodeurl — Encodes lone '%' as %25 while preserving valid %20", () => {
        let out: [128]u8 = undefined;
        let n = encodeUrl("/100% pure%20gold%2", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "/100%25%20pure%20gold%252"), true);
    });

    test("5. @cap/encodeurl — Encodes UTF-8 non-ASCII bytes", () => {
        let out: [128]u8 = undefined;
        let n = encodeUrl("/café", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "/caf%C3%A9"), true);
        expect_eq(isEncodedUrl("/café"), false);
    });

    test("6. @cap/encodeurl — Encodes CRLF control characters (header injection defense)", () => {
        let out: [128]u8 = undefined;
        let n = encodeUrl("/path\r\nX-Injected: 1", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "/path%0D%0AX-Injected:%201"), true);
    });

    test("7. @cap/encodeurl — encodedUrlLen & encodeUrlChecked buffer guard", () => {
        let raw = "/a b/café";
        let expected = "/a%20b/caf%C3%A9";
        expect_eq(encodedUrlLen(raw), expected.len);

        let small: [8]u8 = undefined;
        expect_eq(encodeUrlChecked(raw, small[0..8]) == null, true);

        let full: [32]u8 = undefined;
        let res = encodeUrlChecked(raw, full[0..32]);
        expect_eq(res != null, true);
        let n = res ?? 0;
        expect_eq(eqBuf(full[0..n], n, expected), true);
    });

    test("8. @cap/encodeurl — encodeUrlNormalized uppercases existing %xx hex & IPv6 brackets preserved", () => {
        let out: [128]u8 = undefined;
        let n1 = encodeUrlNormalized("http://[::1]:8080/a%2f%3a b", out[0..128]);
        expect_eq(eqBuf(out[0..n1], n1, "http://[::1]:8080/a%2F%3A%20b"), true);
        expect_eq(needsUrlEncode('['), false);
        expect_eq(needsUrlEncode(']'), false);
        expect_eq(needsUrlEncode('{'), true);
        expect_eq(needsUrlEncode(0x7f), true);
    });

    test("9. @cap/encodeurl — decodeUrl round-trip & malformed percent preservation", () => {
        let enc: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;
        let orig = "/café/hello world?x=<1>";
        let en = encodeUrl(orig, enc[0..128]);
        let dn = decodeUrl(enc[0..en], dec_buf[0..128]);
        expect_eq(eqBuf(dec_buf[0..dn], dn, orig), true);

        let dn2 = decodeUrl("100%25%ZZ%", dec_buf[0..128]);
        expect_eq(eqBuf(dec_buf[0..dn2], dn2, "100%%ZZ%"), true);
    });
}
