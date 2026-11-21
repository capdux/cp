import { test, expect_eq } from "cap:test";
import { NoProxyIpv4Addr as Ipv4Addr, NoProxyIpv4Cidr as Ipv4Cidr, shouldBypassProxy } from "cap:net";

test("1. Ipv4Addr parsing, toU32, loopback, private, and malformed rejection", () => {
    const lo = Ipv4Addr.parse("127.0.0.1") ?? unreachable;
    expect_eq(lo.toU32(), u32(0x7F00_0001));
    expect_eq(lo.isLoopback(), true);
    expect_eq(lo.isPrivate(), false);

    const p10 = Ipv4Addr.parse("10.255.0.1") ?? unreachable;
    expect_eq(p10.isPrivate(), true);
    const p172a = Ipv4Addr.parse("172.16.0.1") ?? unreachable;
    const p172b = Ipv4Addr.parse("172.31.255.254") ?? unreachable;
    const pub172 = Ipv4Addr.parse("172.32.0.1") ?? unreachable;
    expect_eq(p172a.isPrivate(), true);
    expect_eq(p172b.isPrivate(), true);
    expect_eq(pub172.isPrivate(), false);
    const p192 = Ipv4Addr.parse("192.168.10.20") ?? unreachable;
    expect_eq(p192.isPrivate(), true);

    // 畸形输入拦截
    expect_eq(Ipv4Addr.parse("") == null, true);
    expect_eq(Ipv4Addr.parse("1.2.3") == null, true);
    expect_eq(Ipv4Addr.parse("1.2.3.4.5") == null, true);
    expect_eq(Ipv4Addr.parse("256.0.0.1") == null, true);
    expect_eq(Ipv4Addr.parse("01.2.3.4") == null, true);
    expect_eq(Ipv4Addr.parse("1.2..4") == null, true);
    expect_eq(Ipv4Addr.parse("1.2.3.a") == null, true);
});

test("2. Ipv4Cidr /0, /8, /24, /32 subnet containment and invalid CIDR rejection", () => {
    const all = Ipv4Cidr.parse("0.0.0.0/0") ?? unreachable;
    const net8 = Ipv4Cidr.parse("10.0.0.0/8") ?? unreachable;
    const net24 = Ipv4Cidr.parse("192.168.1.123/24") ?? unreachable;
    const host32 = Ipv4Cidr.parse("198.51.100.7/32") ?? unreachable;

    const ip_a = Ipv4Addr.parse("10.99.88.77") ?? unreachable;
    const ip_b = Ipv4Addr.parse("192.168.1.254") ?? unreachable;
    const ip_c = Ipv4Addr.parse("192.168.2.1") ?? unreachable;
    const ip_d = Ipv4Addr.parse("198.51.100.7") ?? unreachable;
    const ip_e = Ipv4Addr.parse("198.51.100.8") ?? unreachable;

    expect_eq(all.contains(ip_a), true);
    expect_eq(all.contains(ip_e), true);
    expect_eq(net8.contains(ip_a), true);
    expect_eq(net8.contains(ip_b), false);
    expect_eq(net24.contains(ip_b), true);
    expect_eq(net24.contains(ip_c), false);
    expect_eq(host32.contains(ip_d), true);
    expect_eq(host32.contains(ip_e), false);

    // 畸形 CIDR 拦截
    expect_eq(Ipv4Cidr.parse("10.0.0.0") == null, true);
    expect_eq(Ipv4Cidr.parse("10.0.0.0/") == null, true);
    expect_eq(Ipv4Cidr.parse("10.0.0.0/33") == null, true);
    expect_eq(Ipv4Cidr.parse("10.0.0.0/08") == null, true);
    expect_eq(Ipv4Cidr.parse("999.0.0.0/8") == null, true);
});

test("3. shouldBypassProxy wildcard *, exact host, domain suffix & anti-false-positive", () => {
    // 全局通配
    expect_eq(shouldBypassProxy("*", "anything.example.org", 443), true);
    expect_eq(shouldBypassProxy("foo.com, * , bar.com", "1.2.3.4", 80), true);

    // 域名后缀（example.com / .example.com / *.example.com）与大小写无关
    const env = "localhost, example.com, .corp.local, *.svc.internal";
    expect_eq(shouldBypassProxy(env, "LOCALHOST", 80), true);
    expect_eq(shouldBypassProxy(env, "example.com", 443), true);
    expect_eq(shouldBypassProxy(env, "api.EXAMPLE.com", 443), true);
    expect_eq(shouldBypassProxy(env, "a.b.corp.local", 8080), true);
    expect_eq(shouldBypassProxy(env, "corp.local", 8080), true);
    expect_eq(shouldBypassProxy(env, "db.svc.internal", 5432), true);

    // 严格防止 badexample.com / mycorp.local 误匹配
    expect_eq(shouldBypassProxy(env, "badexample.com", 443), false);
    expect_eq(shouldBypassProxy(env, "notcorp.local", 8080), false);
    expect_eq(shouldBypassProxy(env, "evilsvc.internal", 443), false);
    // 防止单段规则误匹配 IPv4 地址
    expect_eq(shouldBypassProxy("1", "10.0.0.1", 80), false);
});

test("4. shouldBypassProxy port-scoped rules and IPv6 bracket stripping", () => {
    const env = "internal.corp:8443, [::1], [fe80::1]:9000, ::2";

    expect_eq(shouldBypassProxy(env, "internal.corp", 8443), true);
    expect_eq(shouldBypassProxy(env, "sub.internal.corp", 8443), true);
    expect_eq(shouldBypassProxy(env, "internal.corp", 443), false);

    // IPv6 带括号与不带括号均能匹配
    expect_eq(shouldBypassProxy(env, "[::1]", 80), true);
    expect_eq(shouldBypassProxy(env, "::1", 443), true);
    expect_eq(shouldBypassProxy(env, "[fe80::1]", 9000), true);
    expect_eq(shouldBypassProxy(env, "fe80::1", 9000), true);
    expect_eq(shouldBypassProxy(env, "[fe80::1]", 9001), false);
    expect_eq(shouldBypassProxy(env, "[::2]", 80), true);
});

test("5. shouldBypassProxy IPv4 CIDR rules and empty/malformed rule resilience", () => {
    const env = " , 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, bad:port, [broken, 10.0.0.0/99, valid.host ";

    expect_eq(shouldBypassProxy(env, "10.1.2.3", 80), true);
    expect_eq(shouldBypassProxy(env, "172.20.5.6", 443), true);
    expect_eq(shouldBypassProxy(env, "172.32.5.6", 443), false);
    expect_eq(shouldBypassProxy(env, "192.168.100.1", 8080), true);
    expect_eq(shouldBypassProxy(env, "8.8.8.8", 443), false);
    expect_eq(shouldBypassProxy(env, "valid.host.", 443), true);

    // 空规则或空主机安全返回 false
    expect_eq(shouldBypassProxy("", "localhost", 80), false);
    expect_eq(shouldBypassProxy("   , , ", "localhost", 80), false);
    expect_eq(shouldBypassProxy("*", "", 80), false);
    expect_eq(shouldBypassProxy("*", "   ", 80), false);
});
