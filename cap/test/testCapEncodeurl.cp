import { test, expect_eq } from "cap:test";
import { encodeUrl, isEncodedUrl, escapeRegExp } from "../../packages/@cap/encodeurl/src/index.cp";

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
    test("1. @cap/encodeurl — Valid ASCII RFC 3986 URL preserved verbatim", () => {
        const url = "https://example.com:8080/a/b-c_d.e~f?k=1&v=2#frag";
        let buf: [128]u8 = undefined;
        const n = encodeUrl(url, buf[0..128]);
        expect_eq(n, url.len as i64);
        expect_eq(sliceEq(buf[0..n as usize], url), true);
    });

    test("2. @cap/encodeurl — Existing %20 & %E4%B8%AD are not double-encoded", () => {
        const url = "/search?q=hello%20world&lang=%E4%B8%AD";
        let buf: [128]u8 = undefined;
        const n = encodeUrl(url, buf[0..128]);
        expect_eq(n, url.len as i64);
        expect_eq(sliceEq(buf[0..n as usize], url), true);
    });

    test("3. @cap/encodeurl — Unpaired '%' and invalid '%2G' encoded as '%25'", () => {
        const url = "/discount/50%/bad%2G/end%";
        let buf: [128]u8 = undefined;
        const n = encodeUrl(url, buf[0..128]);
        const expected = "/discount/50%25/bad%252G/end%25";
        expect_eq(n, expected.len as i64);
        expect_eq(sliceEq(buf[0..n as usize], expected), true);
    });

    test("4. @cap/encodeurl — Spaces, CRLF header-splitting chars & UTF-8 bytes encoded uppercase", () => {
        // Contains space, \r\n, <>, and UTF-8 bytes for "中" (0xE4, 0xB8, 0xAD)
        const raw: [10]u8 = [47, 97, 32, 13, 10, 60, 62, 0xE4, 0xB8, 0xAD];
        let buf: [64]u8 = undefined;
        const n = encodeUrl(raw[0..10], buf[0..64]);
        const expected = "/a%20%0D%0A%3C%3E%E4%B8%AD";
        expect_eq(n, expected.len as i64);
        expect_eq(sliceEq(buf[0..n as usize], expected), true);
    });

    test("5. @cap/encodeurl — isEncodedUrl validation", () => {
        expect_eq(isEncodedUrl("https://cp.dev/docs/v1?q=hello%20world"), true);
        expect_eq(isEncodedUrl("/a/b%E4%B8%AD"), true);
        expect_eq(isEncodedUrl("/has space"), false);
        expect_eq(isEncodedUrl("/bad%2G"), false);
        expect_eq(isEncodedUrl("/trailing%"), false);
        expect_eq(isEncodedUrl("/crlf\r\n"), false);
    });

    test("6. @cap/encodeurl — escapeRegExp all metacharacters & undersized buffer -1", () => {
        const src = "a.b*c+d?^e$f(g)h|i{j}[k]l\\m-n";
        let buf: [128]u8 = undefined;
        const n = escapeRegExp(src, buf[0..128]);
        const expected = "a\\.b\\*c\\+d\\?\\^e\\$f\\(g\\)h\\|i\\{j\\}\\[k\\]l\\\\m\\-n";
        expect_eq(n, expected.len as i64);
        expect_eq(sliceEq(buf[0..n as usize], expected), true);

        let tiny: [4]u8 = undefined;
        expect_eq(escapeRegExp(src, tiny[0..4]), -1 as i64);
        expect_eq(encodeUrl("/hello world", tiny[0..4]), -1 as i64);
    });
}
