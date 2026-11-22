// cap/test/test_ws_client_upgrade_production_deep.cp
// 深度生产级回归测试：cap:ws 客户端 URL 解析与 HTTP/1.1 升级状态机 (lib/ws/client_upgrade.cp)

import { test } from "cap:test";
import {
    WS_CLIENT_MAX_PROTOCOLS,
    WsUrlScheme,
    WsUpgradePhase,
    WsClientUrl,
    WsClientUpgradeConfig,
    WsClientUpgradeMachine,
} from "cap:ws";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSlice(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEq(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("ws_client_upgrade: ws/wss URL parsing, IPv6, default/explicit ports & canonical round-trip") {
    let buf: [128]u8 = [0; 128];

    // 1. wss 默认端口 443 + path + query
    const u1 = WsClientUrl.parse("WSS://echo.example.com/chat?room=42")?;
    if (!u1.isTls() || u1.scheme != WsUrlScheme.Wss || u1.port != 443 || !u1.isDefaultPort()) {
        return err.WssDefaultPortMismatch;
    }
    if (!bytesEq(u1.host, "echo.example.com") || !bytesEq(u1.resource_path, "/chat?room=42")) {
        return err.WssHostPathMismatch;
    }
    const h1 = u1.formatHostHeader(buf[0..])?;
    if (!bytesEq(buf[0..h1], "echo.example.com")) return err.HostHeaderDefaultPortOmitFailed;
    const c1 = u1.formatCanonicalUrl(buf[0..])?;
    if (!bytesEq(buf[0..c1], "wss://echo.example.com/chat?room=42")) return err.CanonicalUrl1Failed;

    // 2. IPv6 字面量 + 非默认端口 8443 + 缺省根路径
    const u2 = WsClientUrl.parse("ws://[::1]:8443")?;
    if (u2.isTls() || !u2.is_ipv6 || !bytesEq(u2.host, "::1") || u2.port != 8443 || !bytesEq(u2.resource_path, "/")) {
        return err.Ipv6ParseFailed;
    }
    const h2 = u2.formatHostHeader(buf[0..])?;
    if (!bytesEq(buf[0..h2], "[::1]:8443")) return err.Ipv6HostHeaderFailed;
    const c2 = u2.formatCanonicalUrl(buf[0..])?;
    if (!bytesEq(buf[0..c2], "ws://[::1]:8443/")) return err.CanonicalUrl2Failed;

    // 3. 规范 URL 再次 parse 形成无损 round-trip
    const u2_rt = WsClientUrl.parse(buf[0..c2])?;
    if (!bytesEq(u2_rt.host, u2.host) || u2_rt.port != u2.port || u2_rt.is_ipv6 != u2.is_ipv6) {
        return err.UrlRoundTripMismatch;
    }
}

test("ws_client_upgrade: RFC 6455 opening handshake request build & 101 verification round-trip") {
    const url = WsClientUrl.parse("wss://realtime.example.com:9443/v1/ws")?;
    let cfg = WsClientUpgradeConfig{
        origin: "https://app.example.com",
        enable_permessage_deflate: true,
    };
    cfg.addProtocol("chat.v1")?;
    cfg.addProtocol("superchat")?;
    cfg.addHeader("X-Trace-Id", "req-777")?;

    let machine = WsClientUpgradeMachine.init(url);
    if (machine.phase != WsUpgradePhase.Idle) return err.ExpectedIdlePhase;

    // 使用 RFC 6455 官方测试向量 Key: "dGhlIHNhbXBsZSBub25jZQ==" -> Accept: "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    let req_buf: [1024]u8 = [0; 1024];
    const req_len = machine.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, req_buf[0..])?;
    const req = req_buf[0..req_len];
    if (machine.phase != WsUpgradePhase.AwaitingResponse) return err.ExpectedAwaitingPhase;

    if (!containsSlice(req, "GET /v1/ws HTTP/1.1\r\n")) return err.MissingRequestLine;
    if (!containsSlice(req, "Host: realtime.example.com:9443\r\n")) return err.MissingHostHeader;
    if (!containsSlice(req, "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n")) return err.MissingKeyHeader;
    if (!containsSlice(req, "Origin: https://app.example.com\r\n")) return err.MissingOriginHeader;
    if (!containsSlice(req, "Sec-WebSocket-Protocol: chat.v1, superchat\r\n")) return err.MissingProtocolHeader;
    if (!containsSlice(req, "Sec-WebSocket-Extensions: permessage-deflate")) return err.MissingExtHeader;
    if (!containsSlice(req, "X-Trace-Id: req-777\r\n")) return err.MissingCustomHeader;

    const ok_resp =
        "HTTP/1.1 101 Switching Protocols\r\n" ++
        "Upgrade: WebSocket\r\n" ++
        "Connection: keep-alive, Upgrade\r\n" ++
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n" ++
        "Sec-WebSocket-Protocol: superchat\r\n" ++
        "Sec-WebSocket-Extensions: permessage-deflate\r\n" ++
        "\r\n";

    machine.verifyUpgradeResponse(ok_resp, cfg)?;
    if (machine.phase != WsUpgradePhase.Upgraded) return err.ExpectedUpgradedPhase;
    if (!bytesEq(machine.negotiatedProtocol(), "superchat")) return err.NegotiatedProtocolMismatch;
    if (!machine.has_permessage_deflate) return err.ExpectedDeflateNegotiated;
}

test("ws_client_upgrade: reject wrong status code, forged Sec-WebSocket-Accept & unrequested protocol") {
    const url = WsClientUrl.parse("ws://localhost:8080/ws")?;
    let cfg = WsClientUpgradeConfig{};
    cfg.addProtocol("v1")?;

    // 1. 非 101 状态码拒绝
    let m1 = WsClientUpgradeMachine.init(url);
    let buf: [512]u8 = [0; 512];
    * = m1.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, buf[0..])?;
    let caught_status = false;
    m1.verifyUpgradeResponse("HTTP/1.1 200 OK\r\n\r\n", cfg) catch {
        caught_status = true;
    };
    if (!caught_status || m1.phase != WsUpgradePhase.Failed) return err.ExpectedRejectNon101;

    // 2. 篡改 Sec-WebSocket-Accept 拒绝
    let m2 = WsClientUpgradeMachine.init(url);
    * = m2.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, buf[0..])?;
    const bad_accept =
        "HTTP/1.1 101 Switching Protocols\r\n" ++
        "Upgrade: websocket\r\n" ++
        "Connection: Upgrade\r\n" ++
        "Sec-WebSocket-Accept: AAAAAAAAAAAAAAAAAAAAAAAAAAA=\r\n\r\n";
    let caught_accept = false;
    m2.verifyUpgradeResponse(bad_accept, cfg) catch {
        caught_accept = true;
    };
    if (!caught_accept || m2.phase != WsUpgradePhase.Failed) return err.ExpectedRejectBadAccept;

    // 3. 服务端返回未在候选列表中的子协议拒绝
    let m3 = WsClientUpgradeMachine.init(url);
    * = m3.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, buf[0..])?;
    const bad_proto =
        "HTTP/1.1 101 Switching Protocols\r\n" ++
        "Upgrade: websocket\r\n" ++
        "Connection: Upgrade\r\n" ++
        "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n" ++
        "Sec-WebSocket-Protocol: evil-proto\r\n\r\n";
    let caught_proto = false;
    m3.verifyUpgradeResponse(bad_proto, cfg) catch {
        caught_proto = true;
    };
    if (!caught_proto || m3.phase != WsUpgradePhase.Failed) return err.ExpectedRejectBadProtocol;
}

test("ws_client_upgrade: RFC 6455 URL fragment, scheme, host & port malformed input defense") {
    const bad_urls: [8]([]const u8) = [
        "http://example.com/ws",
        "wss://example.com/ws#frag",
        "ws://",
        "ws://example.com:0/ws",
        "ws://example.com:70000/ws",
        "ws://[::1/ws",
        "ws://user@example.com/ws",
        "ws://example.com/a\r\nb",
    ];
    let i: usize = 0;
    while (i < bad_urls.len) {
        let caught = false;
        * = WsClientUrl.parse(bad_urls[i]) catch {
            caught = true;
        };
        if (!caught) return err.ExpectedMalformedUrlReject;
        i += 1;
    }
}

test("ws_client_upgrade: CRLF header injection & reserved handshake header override defense") {
    let cfg = WsClientUpgradeConfig{};

    // 1. CRLF 注入拦截
    let caught_crlf = false;
    cfg.addHeader("X-Test", "ok\r\nX-Injected: yes") catch {
        caught_crlf = true;
    };
    if (!caught_crlf) return err.ExpectedCrlfReject;

    // 2. 禁止覆盖保留握手头（大小写不敏感）
    const reserved: [5]([]const u8) = ["Host", "upgrade", "CONNECTION", "Sec-WebSocket-Key", "sec-websocket-version"];
    let i: usize = 0;
    while (i < reserved.len) {
        let caught = false;
        cfg.addHeader(reserved[i], "forged") catch {
            caught = true;
        };
        if (!caught) return err.ExpectedReservedHeaderReject;
        i += 1;
    }
}

test("ws_client_upgrade: state transition violation & capacity overflow guards") {
    const url = WsClientUrl.parse("ws://127.0.0.1:9000/s")?;
    let cfg = WsClientUpgradeConfig{};

    // 1. 超过 WS_CLIENT_MAX_PROTOCOLS 拒绝
    let p: usize = 0;
    while (p < WS_CLIENT_MAX_PROTOCOLS) {
        cfg.addProtocol("p1")?;
        p += 1;
    }
    let caught_proto_cap = false;
    cfg.addProtocol("overflow") catch {
        caught_proto_cap = true;
    };
    if (!caught_proto_cap) return err.ExpectedTooManyProtocols;

    // 2. Idle 阶段直接调用 verifyUpgradeResponse 拒绝
    let m = WsClientUpgradeMachine.init(url);
    let caught_state = false;
    m.verifyUpgradeResponse("HTTP/1.1 101 Switching Protocols\r\n\r\n", cfg) catch {
        caught_state = true;
    };
    if (!caught_state) return err.ExpectedInvalidStateOnVerify;

    // 3. 输出缓冲过小安全返回 BufferTooSmall
    let tiny: [32]u8 = [0; 32];
    let caught_small = false;
    * = m.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", cfg, tiny[0..]) catch {
        caught_small = true;
    };
    if (!caught_small) return err.ExpectedBufferTooSmall;
}

test("ws_client_upgrade: query-only resource path normalized with leading slash") {
    const url = WsClientUrl.parse("ws://example.com?token=abc")?;
    let buf: [256]u8 = [0; 256];
    const canon_len = url.formatCanonicalUrl(buf[0..])?;
    if (!bytesEq(buf[0..canon_len], "ws://example.com/?token=abc")) return err.QueryOnlyCanonicalFailed;

    let m = WsClientUpgradeMachine.init(url);
    const req_len = m.buildUpgradeRequest("dGhlIHNhbXBsZSBub25jZQ==", WsClientUpgradeConfig{}, buf[0..])?;
    if (!containsSlice(buf[0..req_len], "GET /?token=abc HTTP/1.1\r\n")) {
        return err.QueryOnlyRequestTargetFailed;
    }
}
