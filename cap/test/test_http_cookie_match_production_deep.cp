import std from "std";
import { test, expect_eq } from "cap:test";
import {
    defaultCookiePath,
    cookieDomainMatches,
    cookiePathMatches,
    CookiePrefixKind,
    detectCookiePrefix,
    validateCookiePrefix,
    canSendCookie,
} from "cap:http";

test("1. defaultCookiePath strictly follows RFC 6265 section 5.1.4", () => {
    expect_eq(std.mem.eql(u8, defaultCookiePath(""), "/"), true);
    expect_eq(std.mem.eql(u8, defaultCookiePath("relative/path"), "/"), true);
    expect_eq(std.mem.eql(u8, defaultCookiePath("/"), "/"), true);
    expect_eq(std.mem.eql(u8, defaultCookiePath("/index.html"), "/"), true);
    expect_eq(std.mem.eql(u8, defaultCookiePath("/a/b/c"), "/a/b"), true);
    expect_eq(std.mem.eql(u8, defaultCookiePath("/a/b/"), "/a/b"), true);
});

test("2. cookieDomainMatches enforces host_only, dot-boundary suffix, and IP literal exact match", () => {
    expect_eq(cookieDomainMatches("Example.COM", "example.com", true), true);
    expect_eq(cookieDomainMatches("sub.example.com", "example.com", true), false);
    expect_eq(cookieDomainMatches("sub.example.com", ".example.com", false), true);
    expect_eq(cookieDomainMatches("a.b.example.com", "EXAMPLE.COM", false), true);
    expect_eq(cookieDomainMatches("badexample.com", "example.com", false), false);

    // IPv4 与 IPv6 字面量禁止后缀匹配
    expect_eq(cookieDomainMatches("192.168.1.1", "192.168.1.1", false), true);
    expect_eq(cookieDomainMatches("192.168.1.1", "168.1.1", false), false);
    expect_eq(cookieDomainMatches("[::1]", "::1", true), true);
    expect_eq(cookieDomainMatches("[::1]", "1", false), false);
});

test("3. cookiePathMatches enforces slash-boundary prefix matching", () => {
    expect_eq(cookiePathMatches("/admin", "/admin"), true);
    expect_eq(cookiePathMatches("/admin/users", "/admin"), true);
    expect_eq(cookiePathMatches("/admin/users", "/admin/"), true);
    expect_eq(cookiePathMatches("/anything", "/"), true);

    // 防止 /admin2 误匹配 /admin
    expect_eq(cookiePathMatches("/admin2", "/admin"), false);
    expect_eq(cookiePathMatches("/adm", "/admin"), false);
    expect_eq(cookiePathMatches("/a", ""), false);
});

test("4. detectCookiePrefix and validateCookiePrefix enforce __Secure- and __Host- rules", () => {
    expect_eq(detectCookiePrefix("sid") == CookiePrefixKind.none, true);
    expect_eq(detectCookiePrefix("__Secure-sid") == CookiePrefixKind.secure_prefix, true);
    expect_eq(detectCookiePrefix("__Host-sid") == CookiePrefixKind.host_prefix, true);

    // __Secure- 要求 Secure=true
    expect_eq(validateCookiePrefix("__Secure-sid", true, true, "/app"), true);
    expect_eq(validateCookiePrefix("__Secure-sid", false, true, "/app"), false);

    // __Host- 要求 Secure=true, 无 Domain 属性, Path="/"
    expect_eq(validateCookiePrefix("__Host-sid", true, false, "/"), true);
    expect_eq(validateCookiePrefix("__Host-sid", false, false, "/"), false);
    expect_eq(validateCookiePrefix("__Host-sid", true, true, "/"), false);
    expect_eq(validateCookiePrefix("__Host-sid", true, false, "/app"), false);
});

test("5. canSendCookie combines HTTPS, HttpOnly, Domain, and Path policies", () => {
    // 全部满足：HTTPS + HTTP API + 子域匹配 + 路径匹配
    expect_eq(
        canSendCookie("api.cp.dev", "/v1/items", true, true, ".cp.dev", false, "/v1", true, true),
        true
    );
    // Secure Cookie 在明文 HTTP 下拦截
    expect_eq(
        canSendCookie("api.cp.dev", "/v1/items", false, true, ".cp.dev", false, "/v1", true, false),
        false
    );
    // HttpOnly Cookie 在非 HTTP API（如 document.cookie）下拦截
    expect_eq(
        canSendCookie("api.cp.dev", "/v1/items", true, false, ".cp.dev", false, "/v1", false, true),
        false
    );
    // 路径不匹配拦截
    expect_eq(
        canSendCookie("api.cp.dev", "/v10/items", true, true, ".cp.dev", false, "/v1", false, false),
        false
    );
});
