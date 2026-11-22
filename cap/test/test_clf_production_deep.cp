// Source: cp/cap/test/test_clf_production_deep.cp (Production deep tests for cap:clf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ClfEntry,
    ClfStats,
    ClfIter,
    isClf,
    parseClfLine,
    summarizeClf,
} from "cap:obs";

const SAMPLE_COMMON_LINE: []const u8 =
    "127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] \"GET /apache_pb.gif HTTP/1.0\" 200 2326";

const SAMPLE_COMBINED_IPV6_LINE: []const u8 =
    "2001:db8::1 - alice [21/Nov/2026:08:15:00 +0000] \"POST /api/v1/search?q=cp&page=2 HTTP/2.0\" 201 4096 \"https://cp-lang.org/docs\" \"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\"";

const SAMPLE_MULTI_LOG: []const u8 =
    "127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] \"GET /apache_pb.gif HTTP/1.0\" 200 2326\r\n" ++
    "2001:db8::1 - alice [21/Nov/2026:08:15:00 +0000] \"GET /redirect HTTP/1.1\" 302 - \"https://example.com/\" \"curl/8.4.0\"\r\n" ++
    "\n" ++
    "10.0.0.5 - - [21/Nov/2026:08:16:00 +0000] \"GET /missing HTTP/1.1\" 404 512 \"-\" \"Bot/1.0\"\n" ++
    "CORRUPTED LOG LINE WITHOUT BRACKETS 200 10\n" ++
    "10.0.0.9 - ops [21/Nov/2026:08:17:00 +0000] \"POST /boom HTTP/1.1\" 503 162\n";

test("1. Standard NCSA Common and IPv6 Apache/Nginx Combined log parsing", () => {
    expect_eq(isClf(SAMPLE_COMMON_LINE), true);

    const c = parseClfLine(SAMPLE_COMMON_LINE) ?? unreachable;
    expect_eq(std.mem.eql(u8, c.host, "127.0.0.1"), true);
    expect_eq(std.mem.eql(u8, c.ident, "-"), true);
    expect_eq(std.mem.eql(u8, c.user, "frank"), true);
    expect_eq(std.mem.eql(u8, c.timestamp, "10/Oct/2000:13:55:36 -0700"), true);
    expect_eq(std.mem.eql(u8, c.request_line, "GET /apache_pb.gif HTTP/1.0"), true);
    expect_eq(std.mem.eql(u8, c.method, "GET"), true);
    expect_eq(std.mem.eql(u8, c.path, "/apache_pb.gif"), true);
    expect_eq(std.mem.eql(u8, c.protocol, "HTTP/1.0"), true);
    expect_eq(c.status, 200);
    expect_eq(c.bytes_sent, 2326);
    expect_eq(c.has_bytes, true);
    expect_eq(c.is_combined, false);
    expect_eq(c.is2xx(), true);
    expect_eq(c.is4xx(), false);

    const comb = parseClfLine(SAMPLE_COMBINED_IPV6_LINE) ?? unreachable;
    expect_eq(std.mem.eql(u8, comb.host, "2001:db8::1"), true);
    expect_eq(std.mem.eql(u8, comb.user, "alice"), true);
    expect_eq(std.mem.eql(u8, comb.method, "POST"), true);
    expect_eq(std.mem.eql(u8, comb.path, "/api/v1/search?q=cp&page=2"), true);
    expect_eq(std.mem.eql(u8, comb.protocol, "HTTP/2.0"), true);
    expect_eq(comb.status, 201);
    expect_eq(comb.bytes_sent, 4096);
    expect_eq(comb.is_combined, true);
    expect_eq(std.mem.eql(u8, comb.referer, "https://cp-lang.org/docs"), true);
    expect_eq(std.mem.eql(u8, comb.user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36"), true);
});

test("2. Escaped quotes (\\\") in request/UA and dash ('-') bytes_sent handling", () => {
    const escaped_line =
        "192.168.1.10 - - [21/Nov/2026:09:00:00 +0000] \"GET /q=\\\"hello\\\" HTTP/1.1\" 304 - \"-\" \"Custom \\\"Quoted\\\" Agent\"";
    const e = parseClfLine(escaped_line) ?? unreachable;
    expect_eq(std.mem.eql(u8, e.method, "GET"), true);
    expect_eq(std.mem.eql(u8, e.path, "/q=\\\"hello\\\""), true);
    expect_eq(std.mem.eql(u8, e.protocol, "HTTP/1.1"), true);
    expect_eq(e.status, 304);
    expect_eq(e.is3xx(), true);
    expect_eq(e.bytes_sent, 0);
    expect_eq(e.has_bytes, false);
    expect_eq(e.is_combined, true);
    expect_eq(std.mem.eql(u8, e.user_agent, "Custom \\\"Quoted\\\" Agent"), true);
});

test("3. Request line splitting for HTTP/0.9 (no protocol) and single-token requests", () => {
    const http09 = "127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] \"GET /legacy\" 200 100";
    const e09 = parseClfLine(http09) ?? unreachable;
    expect_eq(std.mem.eql(u8, e09.method, "GET"), true);
    expect_eq(std.mem.eql(u8, e09.path, "/legacy"), true);
    expect_eq(e09.protocol.len, 0);

    const bad_req = "127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] \"-\" 400 0";
    const eb = parseClfLine(bad_req) ?? unreachable;
    expect_eq(std.mem.eql(u8, eb.method, "-"), true);
    expect_eq(eb.path.len, 0);
    expect_eq(eb.is4xx(), true);
});

test("4. ClfIter and summarizeClf aggregate 2xx/3xx/4xx/5xx, bytes, combined_count, and malformed_lines", () => {
    const stats = summarizeClf(SAMPLE_MULTI_LOG);
    expect_eq(stats.total_requests, 4);
    expect_eq(stats.status_2xx, 1);
    expect_eq(stats.status_3xx, 1);
    expect_eq(stats.status_4xx, 1);
    expect_eq(stats.status_5xx, 1);
    // 2326 + 0 + 512 + 162 = 3000
    expect_eq(stats.total_bytes_sent, 3000);
    expect_eq(stats.combined_count, 2);
    expect_eq(stats.malformed_lines, 1);

    let it = ClfIter.init(SAMPLE_MULTI_LOG);
    let n: u32 = 0;
    while (entry := it.next()) {
        * = entry.status;
        n += 1;
    }
    expect_eq(n, 4);
    expect_eq(it.skipped_malformed, 1);
});

test("5. Resilience against missing [timestamp], invalid status codes, unclosed quotes, and empty input", () => {
    expect_eq(isClf(""), false);
    expect_eq(isClf("   \r\n\n"), false);
    expect_eq(parseClfLine("127.0.0.1 - - 10/Oct/2000 \"GET / HTTP/1.1\" 200 10") == null, true); // 缺方括号
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1 200 10") == null, true);            // 未闭合请求引号
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1\" 99 10") == null, true);          // 状态码 < 100
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1\" 600 10") == null, true);         // 状态码 > 599
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1\" abc 10") == null, true);         // 非数字状态码
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1\" 200 10 \"unclosed") == null, true);
    expect_eq(parseClfLine("127.0.0.1 - - [ts] \"GET / HTTP/1.1\" 200 10 \"only_ref\"") == null, true);
});
