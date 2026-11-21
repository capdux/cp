import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UpgradeIter,
    ConnectionTokenIter,
    connectionHasToken,
    upgradeHasProtocol,
    isHopByHopHeader,
    isWebSocketUpgrade,
} from "cap:http";

test("1. UpgradeIter parses protocol/version lists and skips empty/malformed entries", () => {
    const hdr = "HTTP/2.0, , SHTTP/1.3, IRC/6.9, websocket, bad/proto/extra";
    let it = UpgradeIter.init(hdr);

    const p0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p0.name, "HTTP"), true);
    expect_eq(std.mem.eql(u8, p0.version ?? unreachable, "2.0"), true);

    const p1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.name, "SHTTP"), true);
    expect_eq(std.mem.eql(u8, p1.version ?? unreachable, "1.3"), true);

    const p2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.name, "IRC"), true);
    expect_eq(std.mem.eql(u8, p2.version ?? unreachable, "6.9"), true);

    const p3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p3.name, "websocket"), true);
    expect_eq(p3.version == null, true);

    expect_eq(it.next() == null, true);
});

test("2. ConnectionTokenIter, connectionHasToken, and upgradeHasProtocol are case-insensitive", () => {
    let it = ConnectionTokenIter.init(" keep-alive , UPGRADE , X-Custom-Hop ");
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "keep-alive"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "UPGRADE"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "X-Custom-Hop"), true);
    expect_eq(it.next() == null, true);

    expect_eq(connectionHasToken("keep-alive, Upgrade", "upgrade"), true);
    expect_eq(connectionHasToken("keep-alive, Upgrade", "KEEP-ALIVE"), true);
    expect_eq(connectionHasToken("keep-alive, Upgrade", "close"), false);

    expect_eq(upgradeHasProtocol("h2c, WebSocket", "websocket"), true);
    expect_eq(upgradeHasProtocol("HTTP/2.0, h2c", "http"), true);
    expect_eq(upgradeHasProtocol("h2c", "websocket"), false);
});

test("3. isHopByHopHeader covers all 8 RFC 9110 standard headers and dynamic Connection headers", () => {
    const std_hops: [8][]const u8 = [
        "Connection",
        "Keep-Alive",
        "Proxy-Authenticate",
        "Proxy-Authorization",
        "TE",
        "Trailer",
        "Transfer-Encoding",
        "Upgrade",
    ];
    for (h of std_hops) {
        expect_eq(isHopByHopHeader(h, null), true);
    }

    // 端到端头默认不是逐跳头
    expect_eq(isHopByHopHeader("Content-Type", null), false);
    expect_eq(isHopByHopHeader("Authorization", "keep-alive, upgrade"), false);

    // 通过 Connection 动态指定的自定义逐跳头
    expect_eq(isHopByHopHeader("X-Proxy-Trace", "keep-alive, x-proxy-trace"), true);
    expect_eq(isHopByHopHeader("X-Proxy-Trace", null), false);
});

test("4. isWebSocketUpgrade validates Connection, Upgrade, and optional Sec-WebSocket-Version=13", () => {
    expect_eq(isWebSocketUpgrade("keep-alive, Upgrade", "WebSocket", "13"), true);
    expect_eq(isWebSocketUpgrade("Upgrade", "h2c, websocket", null), true);
    expect_eq(isWebSocketUpgrade("Upgrade", "websocket", " 13 "), true);

    // 缺失任一条件或版本号非 13 均拒绝
    expect_eq(isWebSocketUpgrade("keep-alive", "websocket", "13"), false);
    expect_eq(isWebSocketUpgrade("Upgrade", "h2c", "13"), false);
    expect_eq(isWebSocketUpgrade("Upgrade", "websocket", "8"), false);
    expect_eq(isWebSocketUpgrade("Upgrade", "websocket", ""), false);
});

test("5. empty and malformed header edge cases", () => {
    expect_eq(connectionHasToken("", "upgrade"), false);
    expect_eq(upgradeHasProtocol("", "websocket"), false);
    expect_eq(isHopByHopHeader("", "upgrade"), false);
    expect_eq(isWebSocketUpgrade("", "", null), false);
});
