import {
    ProxyScheme,
    ProxyUrl,
    ProxyEnv,
    selectProxy,
} from "cap:http";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("ProxyUrl parses http, https, socks5h, credentials, IPv6, and bare host:port", () => {
    const p1 = ProxyUrl.parse("http://alice:s3cr3t@proxy.corp:8080/").?;
    expect(p1.scheme == ProxyScheme.http);
    expect(strEq(p1.username.?, "alice"));
    expect(strEq(p1.password.?, "s3cr3t"));
    expect(strEq(p1.host, "proxy.corp"));
    expect(p1.port == 8080);
    expect(!p1.isSocks() && p1.resolvesDnsRemotely());

    const p2 = ProxyUrl.parse("socks5h://[::1]:1080").?;
    expect(p2.scheme == ProxyScheme.socks5h);
    expect(p2.isSocks() && p2.resolvesDnsRemotely());
    expect(strEq(p2.host, "[::1]") && p2.port == 1080);

    const p3 = ProxyUrl.parse("127.0.0.1:3128").?;
    expect(p3.scheme == ProxyScheme.http && p3.port == 3128);

    expect(ProxyUrl.parse("ftp://bad:21") == null);
    expect(ProxyUrl.parse("http://host:0") == null);
});

test("selectProxy respects HTTPS_PROXY, HTTP_PROXY, ALL_PROXY, and NO_PROXY bypass", () => {
    let env = ProxyEnv.empty();
    env.http_proxy = "http://http-gw:8080";
    env.https_proxy = "http://https-gw:8443";
    env.all_proxy = "socks5://fallback:1080";
    env.no_proxy = "localhost, .internal.corp";

    const r1 = selectProxy(env, "https", "api.github.com").?;
    expect(strEq(r1.host, "https-gw") && r1.port == 8443);

    const r2 = selectProxy(env, "http", "example.com").?;
    expect(strEq(r2.host, "http-gw") && r2.port == 8080);

    // Bypassed by NO_PROXY
    expect(selectProxy(env, "https", "db.internal.corp") == null);
    expect(selectProxy(env, "http", "localhost") == null);

    // Fallback to ALL_PROXY when scheme-specific proxy is unset
    env.https_proxy = null;
    const r3 = selectProxy(env, "https", "api.github.com").?;
    expect(r3.isSocks() && strEq(r3.host, "fallback"));
});
