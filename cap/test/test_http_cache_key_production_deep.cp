import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isTrackingQueryParam,
    normalizeQueryWithoutTracking,
    buildCacheKey,
    appendVaryDimension,
} from "cap:http_cache_key";

test("1. isTrackingQueryParam matches utm_* and known tracking keys case-insensitively", () => {
    expect_eq(isTrackingQueryParam("utm_source"), true);
    expect_eq(isTrackingQueryParam("UTM_MEDIUM"), true);
    expect_eq(isTrackingQueryParam("Utm_Campaign"), true);
    expect_eq(isTrackingQueryParam("fbclid"), true);
    expect_eq(isTrackingQueryParam("FBCLID"), true);
    expect_eq(isTrackingQueryParam("gclid"), true);
    expect_eq(isTrackingQueryParam("mc_cid"), true);
    expect_eq(isTrackingQueryParam("MC_EID"), true);
    expect_eq(isTrackingQueryParam("ref_src"), true);

    expect_eq(isTrackingQueryParam(""), false);
    expect_eq(isTrackingQueryParam("utm"), false);
    expect_eq(isTrackingQueryParam("id"), false);
    expect_eq(isTrackingQueryParam("page"), false);
    expect_eq(isTrackingQueryParam("ref"), false);
});

test("2. normalizeQueryWithoutTracking strips leading ?, empty segments, and tracking params while preserving order", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const q1 = normalizeQueryWithoutTracking(
        "?&&id=42&utm_source=newsletter&page=2&FBCLID=abc&&sort=asc&gclid=999&",
        &buf,
    ) ?? unreachable;
    expect_eq(std.mem.eql(u8, q1, "id=42&page=2&sort=asc"), true);

    // All tracking params -> empty slice
    const q2 = normalizeQueryWithoutTracking("?utm_term=cp&mc_cid=1&ref_src=tw", &buf) ?? unreachable;
    expect_eq(q2.len, usize(0));

    // Tiny buffer returns null
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(normalizeQueryWithoutTracking("a=12345", &tiny) == null, true);
});

test("3. buildCacheKey normalizes method, scheme, host, default ports, path, and tracking query", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    const k1 = buildCacheKey(
        "get",
        "HTTPS",
        "Example.COM:443",
        "/api/v1/items",
        "?id=1&utm_source=x&fbclid=y&lang=zh",
        true,
        &buf,
    ) ?? unreachable;
    expect_eq(std.mem.eql(u8, k1, "GET https://example.com/api/v1/items?id=1&lang=zh"), true);

    // http default port :80 stripped, empty path -> "/"
    const k2 = buildCacheKey("head", "HTTP", "API.Local:80", "", "", true, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, k2, "HEAD http://api.local/"), true);

    // Non-default & cross-protocol ports preserved; strip_tracking=false keeps utm_*
    const k3 = buildCacheKey("get", "http", "example.com:443", "v2/list", "?utm_source=a&id=9", false, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, k3, "GET http://example.com:443/v2/list?utm_source=a&id=9"), true);

    // IPv6 host with default port :443 stripped
    const k4 = buildCacheKey("GET", "https", "[::1]:443", "/", "?utm_campaign=c", true, &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, k4, "GET https://[::1]/"), true);
});

test("4. appendVaryDimension appends normalized Vary lines and preserves atomicity on overflow", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const base = buildCacheKey("get", "https", "cdn.cp.dev:443", "/img.png", "", true, &buf) ?? unreachable;
    let pos: usize = base.len;

    expect_eq(appendVaryDimension(&buf, &pos, "Accept-Encoding", "  br, gzip  "), true);
    expect_eq(appendVaryDimension(&buf, &pos, "Accept-Language", "en-US"), true);
    expect_eq(
        std.mem.eql(
            u8,
            buf[0..pos],
            "GET https://cdn.cp.dev/img.png\naccept-encoding:br, gzip\naccept-language:en-US",
        ),
        true,
    );

    // Overflow must fail atomically without mutating pos
    let small: [40]u8 = std.mem.zeroes([40]u8);
    const sbase = buildCacheKey("get", "https", "a.io", "/", "", true, &small) ?? unreachable;
    let spos: usize = sbase.len;
    const saved = spos;
    expect_eq(appendVaryDimension(&small, &spos, "Accept-Encoding", "very-long-encoding-value-exceeding"), false);
    expect_eq(spos, saved);
});

test("5. Defensive boundary guards reject invalid inputs, control chars, and tiny buffers", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Empty required fields
    expect_eq(buildCacheKey("", "https", "a.io", "/", "", true, &buf) == null, true);
    expect_eq(buildCacheKey("GET", "", "a.io", "/", "", true, &buf) == null, true);
    expect_eq(buildCacheKey("GET", "https", "", "/", "", true, &buf) == null, true);

    // Control / whitespace / CRLF injection rejection
    expect_eq(buildCacheKey("GET ", "https", "a.io\r\n", "/", "", true, &buf) == null, true);
    expect_eq(buildCacheKey("GET", "https", "a.io", "/a\nb", "", true, &buf) == null, true);
    expect_eq(buildCacheKey("GET", "https", "a.io", "/", "x=1\ry=2", true, &buf) == null, true);

    // Tiny output buffer
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    expect_eq(buildCacheKey("GET", "https", "example.com", "/api", "", true, &tiny) == null, true);

    // Invalid Vary header name / value
    let pos: usize = 0;
    expect_eq(appendVaryDimension(&buf, &pos, "", "gzip"), false);
    expect_eq(appendVaryDimension(&buf, &pos, "Bad:Name", "gzip"), false);
    expect_eq(appendVaryDimension(&buf, &pos, "Accept", "gzip\nEvil:1"), false);
    expect_eq(pos, usize(0));
});
