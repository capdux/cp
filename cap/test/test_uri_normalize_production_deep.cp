import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isUnreserved,
    defaultPortForScheme,
    normalizePercentEncoding,
    removeDotSegments,
    normalizeOrigin,
} from "cap:url";

test("1. isUnreserved and defaultPortForScheme cover RFC 3986 and standard schemes", () => {
    expect_eq(isUnreserved('A'), true);
    expect_eq(isUnreserved('z'), true);
    expect_eq(isUnreserved('5'), true);
    expect_eq(isUnreserved('-'), true);
    expect_eq(isUnreserved('.'), true);
    expect_eq(isUnreserved('_'), true);
    expect_eq(isUnreserved('~'), true);
    expect_eq(isUnreserved('/'), false);
    expect_eq(isUnreserved(' '), false);
    expect_eq(isUnreserved('%'), false);

    expect_eq(defaultPortForScheme("HTTP") ?? unreachable, u16(80));
    expect_eq(defaultPortForScheme("ws") ?? unreachable, u16(80));
    expect_eq(defaultPortForScheme("Https") ?? unreachable, u16(443));
    expect_eq(defaultPortForScheme("WSS") ?? unreachable, u16(443));
    expect_eq(defaultPortForScheme("ftp") ?? unreachable, u16(21));
    expect_eq(defaultPortForScheme("ssh") ?? unreachable, u16(22));
    expect_eq(defaultPortForScheme("postgres") ?? unreachable, u16(5432));
    expect_eq(defaultPortForScheme("PostgreSQL") ?? unreachable, u16(5432));
    expect_eq(defaultPortForScheme("mysql") ?? unreachable, u16(3306));
    expect_eq(defaultPortForScheme("redis") ?? unreachable, u16(6379));
    expect_eq(defaultPortForScheme("custom") == null, true);
});

test("2. normalizePercentEncoding decodes unreserved bytes, uppercases reserved escapes, and rejects malformed %", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    const s1 = normalizePercentEncoding("/%7euser/%41%42%2d%5f/%2f%3a%20", &out) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "/~user/AB-_/%2F%3A%20"), true);

    // 非法百分号序列拦截
    expect_eq(normalizePercentEncoding("%", &out) == null, true);
    expect_eq(normalizePercentEncoding("%2", &out) == null, true);
    expect_eq(normalizePercentEncoding("%GG", &out) == null, true);
});

test("3. removeDotSegments conforms strictly to RFC 3986 section 5.2.4", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, removeDotSegments("/a/b/c/./../../g", &out) ?? unreachable, "/a/g"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments("mid/content=5/../6", &out) ?? unreachable, "mid/6"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments("/a/b/..", &out) ?? unreachable, "/a/"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments("/a/b/.", &out) ?? unreachable, "/a/b/"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments("/../../..", &out) ?? unreachable, "/"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments(".././a/b", &out) ?? unreachable, "a/b"), true);
    expect_eq(std.mem.eql(u8, removeDotSegments(".", &out) ?? unreachable, ""), true);
    expect_eq(std.mem.eql(u8, removeDotSegments("..", &out) ?? unreachable, ""), true);
});

test("4. normalizeOrigin lowercases scheme/host and strips default ports while keeping custom ports", () => {
    let out: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, normalizeOrigin("HTTPS", "Example.COM", 443, &out) ?? unreachable, "https://example.com"), true);
    expect_eq(std.mem.eql(u8, normalizeOrigin("HTTP", "LocalHost", 80, &out) ?? unreachable, "http://localhost"), true);
    expect_eq(std.mem.eql(u8, normalizeOrigin("https", "Example.COM", 8443, &out) ?? unreachable, "https://example.com:8443"), true);
    expect_eq(std.mem.eql(u8, normalizeOrigin("postgres", "DB.Internal", 5432, &out) ?? unreachable, "postgres://db.internal"), true);
    expect_eq(std.mem.eql(u8, normalizeOrigin("custom", "Host", 9000, &out) ?? unreachable, "custom://host:9000"), true);
    expect_eq(std.mem.eql(u8, normalizeOrigin("http", "example.com", null, &out) ?? unreachable, "http://example.com"), true);

    expect_eq(normalizeOrigin("", "example.com", 80, &out) == null, true);
    expect_eq(normalizeOrigin("http", "", 80, &out) == null, true);
});

test("5. buffer-too-small safety guards", () => {
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(normalizePercentEncoding("/%2f/a", &tiny) == null, true);
    expect_eq(removeDotSegments("/a/b/c", &tiny) == null, true);
    expect_eq(normalizeOrigin("https", "example.com", 8443, &tiny) == null, true);
});
