import { test, expect_eq } from "cap:test";
import { parseClf, formatClf } from "../../packages/@cap/clf-log/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/clf-log — Standard Apache CLF line parsing", () => {
        const line = "127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] \"GET /apache_pb.gif HTTP/1.0\" 200 2326";
        const r = parseClf(line);
        expect_eq(r.valid, true);
        expect_eq(r.is_combined, false);
        expect_eq(testEq(line[r.host_start..r.host_start + r.host_len], "127.0.0.1"), true);
        expect_eq(testEq(line[r.user_start..r.user_start + r.user_len], "frank"), true);
        expect_eq(testEq(line[r.time_start..r.time_start + r.time_len], "10/Oct/2000:13:55:36 -0700"), true);
        expect_eq(testEq(line[r.method_start..r.method_start + r.method_len], "GET"), true);
        expect_eq(testEq(line[r.path_start..r.path_start + r.path_len], "/apache_pb.gif"), true);
        expect_eq(testEq(line[r.proto_start..r.proto_start + r.proto_len], "HTTP/1.0"), true);
        expect_eq(r.status, 200 as u16);
        expect_eq(r.bytes, 2326 as i64);
    });

    test("2. @cap/clf-log — Full Nginx Combined Log Format with Referer & User-Agent", () => {
        const line = "10.0.0.5 - alice [26/Nov/2026:12:00:00 +0000] \"POST /api/v2/items HTTP/2.0\" 201 512 \"https://cap.dev/app\" \"Mozilla/5.0 (X11; Linux x86_64)\"";
        const r = parseClf(line);
        expect_eq(r.valid, true);
        expect_eq(r.is_combined, true);
        expect_eq(r.status, 201 as u16);
        expect_eq(r.bytes, 512 as i64);
        expect_eq(testEq(line[r.referer_start..r.referer_start + r.referer_len], "https://cap.dev/app"), true);
        expect_eq(testEq(line[r.ua_start..r.ua_start + r.ua_len], "Mozilla/5.0 (X11; Linux x86_64)"), true);
    });

    test("3. @cap/clf-log — Dash bytes '-' (-1) & anonymous user '-'", () => {
        const line = "192.168.1.10 - - [26/Nov/2026:12:05:00 +0000] \"HEAD /health HTTP/1.1\" 304 -";
        const r = parseClf(line);
        expect_eq(r.valid, true);
        expect_eq(testEq(line[r.user_start..r.user_start + r.user_len], "-"), true);
        expect_eq(r.status, 304 as u16);
        expect_eq(r.bytes, -1 as i64);
    });

    test("4. @cap/clf-log — IPv6 client address & query string path", () => {
        const line = "2001:db8::42 - bob [26/Nov/2026:12:10:00 +0000] \"GET /search?q=cp+lang&page=2 HTTP/1.1\" 200 10240\r\n";
        const r = parseClf(line);
        expect_eq(r.valid, true);
        expect_eq(testEq(line[r.host_start..r.host_start + r.host_len], "2001:db8::42"), true);
        expect_eq(testEq(line[r.path_start..r.path_start + r.path_len], "/search?q=cp+lang&page=2"), true);
        expect_eq(r.bytes, 10240 as i64);
    });

    test("5. @cap/clf-log — formatClf -> parseClf roundtrip", () => {
        let buf: [256]u8 = undefined;
        const n1 = formatClf("127.0.0.1", "", "26/Nov/2026:12:15:00 +0000", "DELETE", "/items/9", "HTTP/1.1", 204, -1, buf[0..256]);
        expect_eq(n1 > 0, true);
        const expected = "127.0.0.1 - - [26/Nov/2026:12:15:00 +0000] \"DELETE /items/9 HTTP/1.1\" 204 -";
        expect_eq(testEq(buf[0..n1 as usize], expected), true);

        const parsed = parseClf(buf[0..n1 as usize]);
        expect_eq(parsed.valid, true);
        expect_eq(parsed.status, 204 as u16);
        expect_eq(parsed.bytes, -1 as i64);
    });

    test("6. @cap/clf-log — Malformed quotes, invalid status & undersized buffer -1", () => {
        expect_eq(parseClf("").valid, false);
        expect_eq(parseClf("127.0.0.1 - - [time] GET / HTTP/1.1 200 0").valid, false);
        expect_eq(parseClf("127.0.0.1 - - [time] \"GET / HTTP/1.1\" 999 0").valid, false);
        expect_eq(parseClf("127.0.0.1 - - [time] \"GET / HTTP/1.1\" 200 abc").valid, false);
        expect_eq(parseClf("127.0.0.1 - - [time] \"GET / HTTP/1.1\" 200 10 \"unterminated").valid, false);

        let tiny: [10]u8 = undefined;
        expect_eq(formatClf("127.0.0.1", "u", "t", "GET", "/", "HTTP/1.1", 200, 0, tiny[0..10]), -1 as i64);
    });
}
