import { test, expect } from "cap:test";
import {
    CookieJarSameSite as SameSite,
    Cookie,
    CookieRequestIterator,
    getCookie,
} from "cap:http";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("http_cookie: Set-Cookie parse, domain/path matching, and header serialization", () => {
    let buf: [256]u8 = [0; 256];
    const hdr = "sid=\"abc_123\"; Domain=.example.com; Path=/api; Max-Age=3600; Secure; HttpOnly; Partitioned; SameSite=Lax";
    const c = Cookie.parseSetCookie(hdr).?;

    expect(strEq(c.name, "sid") && strEq(c.value, "abc_123"));
    expect(strEq(c.domain, "example.com")); // leading dot stripped per RFC 6265
    expect(strEq(c.path, "/api"));
    expect(c.max_age.? == 3600);
    expect(c.secure && c.http_only && c.partitioned && c.same_site == SameSite.lax);

    expect(c.matchesDomain("example.com"));
    expect(c.matchesDomain("api.example.com"));
    expect(!c.matchesDomain("badexample.com"));

    expect(c.matchesPath("/api"));
    expect(c.matchesPath("/api/v1/users"));
    expect(!c.matchesPath("/apiv2"));

    const serialized = c.toSetCookieHeader(buf[0..]).?;
    expect(strEq(
        serialized,
        "sid=abc_123; Domain=example.com; Path=/api; Max-Age=3600; Secure; HttpOnly; Partitioned; SameSite=Lax",
    ));

    expect(Cookie.parseSetCookie("bad name=1") == null);
    expect(Cookie.parseSetCookie("no_equals") == null);
});

test("http_cookie: Cookie request header iterator and lookup", () => {
    const req = "theme=dark; session=\"tok=en\"; lang=zh-CN";
    expect(strEq(getCookie(req, "theme").?, "dark"));
    expect(strEq(getCookie(req, "session").?, "tok=en"));
    expect(strEq(getCookie(req, "lang").?, "zh-CN"));
    expect(getCookie(req, "missing") == null);

    let it = CookieRequestIterator.init(req);
    let count: usize = 0;
    while (it.next() != null) : (count += 1) {}
    expect(count == 3);
});
