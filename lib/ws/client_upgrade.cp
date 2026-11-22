// Source: Absorbed from /data/dev/bun/src/http/websocket_http_client.cp | Synced: 2026-11-22T09:28:00Z
// Ported from Bun: /data/dev/bun/src/http/websocket_http_client.cp & /data/dev/bun/docs/runtime/http/websockets.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation RFC 6455 WebSocket Client URL Parser & HTTP/1.1 Upgrade State Machine:
// - Parses ws:// and wss:// URLs (IPv4, IPv6 literals, explicit/default ports, path + query, rejecting #fragment & CRLF)
// - Builds RFC 6455 HTTP/1.1 Upgrade requests with Origin, Sec-WebSocket-Protocol, permessage-deflate & custom headers
// - Strictly verifies HTTP 101 Switching Protocols responses, Sec-WebSocket-Accept digest & negotiated subprotocol
// - Tracks deterministic client upgrade phase transitions (Idle -> AwaitingResponse -> Upgraded / Failed).

import { WebSocketHandshake } from "./handshake";

/// 客户端升级请求支持的最大自定义 HTTP 头数量
pub const WS_CLIENT_MAX_CUSTOM_HEADERS: usize = 16;

/// 客户端升级请求支持的最大候选子协议数量
pub const WS_CLIENT_MAX_PROTOCOLS: usize = 8;

/// WebSocket 客户端 URL 与升级握手错误集
pub err WsClientUpgradeError {
    InvalidScheme,
    InvalidHost,
    InvalidPort,
    FragmentNotAllowed,
    InvalidHeader,
    TooManyHeaders,
    TooManyProtocols,
    BufferTooSmall,
    UpgradeRejected,
    InvalidAcceptKey,
    ProtocolMismatch,
    InvalidState,
}

/// WebSocket 传输协议类型（对应 Bun `WebSocketHTTPClient` / `WebSocketHTTPSClient`）
pub enum WsUrlScheme: u8 {
    Ws = 0,
    Wss = 1,
}

/// 客户端升级握手阶段状态机
pub enum WsUpgradePhase: u8 {
    Idle = 0,
    AwaitingResponse = 1,
    Upgraded = 2,
    Failed = 3,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + (32 as u8);
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEqIgnoreCase(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

fn hasControlOrCrlf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch == '\r' || ch == '\n' || ch == 0 || ch < 0x20 || ch == 0x7F) return true;
        i += 1;
    }
    return false;
}

fn isValidHttpToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
            c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
            c == '\'' || c == '*' || c == '+' || c == '^' || c == '`' ||
            c == '|' || c == '~';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

fn parsePortStrict(s: []const u8): u16! {
    if (s.len == 0 || s.len > 5) return err.InvalidPort;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return err.InvalidPort;
        acc = acc * 10 + ((c - '0') as u32);
        if (acc > 65535) return err.InvalidPort;
        i += 1;
    }
    if (acc == 0) return err.InvalidPort;
    return @intCast(acc);
}

fn appendBytes(out: []u8, pos: *usize, data: []const u8): ! {
    if (pos.* + data.len > out.len) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendPort(out: []u8, pos: *usize, port: u16): ! {
    let digits: [5]u8 = [0; 5];
    let d_len: usize = 0;
    let n: u32 = port as u32;
    while (n > 0) {
        const rem: u8 = @intCast(n % 10);
        digits[d_len] = (48 as u8) + rem;
        d_len += 1;
        n = n / 10;
    }
    if (pos.* + d_len > out.len) return err.BufferTooSmall;
    let k: usize = d_len;
    while (k > 0) {
        k -= 1;
        out[pos.*] = digits[k];
        pos.* += 1;
    }
}

/// RFC 6455 WebSocket 客户端 URL 解析视图（零动态堆分配）
pub struct WsClientUrl {
    scheme: WsUrlScheme = .Ws,
    host: []const u8 = "",
    port: u16 = 80,
    has_explicit_port: bool = false,
    resource_path: []const u8 = "/",
    is_ipv6: bool = false,

    /// 解析 `ws://` / `wss://` URL（严格遵循 RFC 6455 Section 3：禁止 `#fragment` 与控制字符）
    static fn parse(raw_url: []const u8): WsClientUrl! {
        if (raw_url.len < 6 || hasControlOrCrlf(raw_url)) {
            return err.InvalidScheme;
        }

        // RFC 6455 Section 3: WebSocket URIs MUST NOT contain a '#' fragment component
        let f_idx: usize = 0;
        while (f_idx < raw_url.len) {
            if (raw_url[f_idx] == '#') return err.FragmentNotAllowed;
            f_idx += 1;
        }

        let scheme = WsUrlScheme.Ws;
        let rest_start: usize = 0;
        let default_port: u16 = 80;

        if (raw_url.len >= 5 && strEqIgnoreCase(raw_url[0..5], "ws://")) {
            scheme = .Ws;
            rest_start = 5;
            default_port = 80;
        } else if (raw_url.len >= 6 && strEqIgnoreCase(raw_url[0..6], "wss://")) {
            scheme = .Wss;
            rest_start = 6;
            default_port = 443;
        } else {
            return err.InvalidScheme;
        }

        const rest = raw_url[rest_start..raw_url.len];
        if (rest.len == 0) return err.InvalidHost;

        // 切分 authority 与 resource_path（首个 '/' 或 '?'）
        let auth_end: usize = rest.len;
        let i: usize = 0;
        while (i < rest.len) {
            if (rest[i] == '/' || rest[i] == '?') {
                auth_end = i;
                break;
            }
            i += 1;
        }

        const authority = rest[0..auth_end];
        if (authority.len == 0) return err.InvalidHost;

        let resource_path: []const u8 = "/";
        if (auth_end < rest.len) {
            if (rest[auth_end] == '/') {
                resource_path = rest[auth_end..rest.len];
            } else {
                // `ws://host?a=1` -> 保留 query（注意 RFC 6455 要求请求目标以 `/` 开头，在构建请求时补 `/`）
                resource_path = rest[auth_end..rest.len];
            }
        }

        // 解析 IPv6 `[::1]:8443` 或常规 `host:port`
        let host: []const u8 = "";
        let port: u16 = default_port;
        let has_explicit_port = false;
        let is_ipv6 = false;

        if (authority[0] == '[') {
            is_ipv6 = true;
            let close_br: usize = 0;
            let found_close = false;
            let j: usize = 1;
            while (j < authority.len) {
                if (authority[j] == ']') {
                    close_br = j;
                    found_close = true;
                    break;
                }
                j += 1;
            }
            if (!found_close || close_br <= 1) return err.InvalidHost;
            host = authority[1..close_br];
            if (close_br + 1 < authority.len) {
                if (authority[close_br + 1] != ':') return err.InvalidHost;
                port = parsePortStrict(authority[close_br + 2..authority.len])?;
                has_explicit_port = true;
            }
        } else {
            let colon_pos: usize? = null;
            let j: usize = 0;
            while (j < authority.len) {
                const c = authority[j];
                if (c == ' ' || c == '@' || c == '[' || c == ']') return err.InvalidHost;
                if (c == ':') {
                    if (colon_pos != null) return err.InvalidHost;
                    colon_pos = j;
                }
                j += 1;
            }
            if (cp := colon_pos) {
                host = authority[0..cp];
                if (host.len == 0) return err.InvalidHost;
                port = parsePortStrict(authority[cp + 1..authority.len])?;
                has_explicit_port = true;
            } else {
                host = authority;
            }
        }

        if (host.len == 0) return err.InvalidHost;

        return WsClientUrl{
            scheme: scheme,
            host: host,
            port: port,
            has_explicit_port: has_explicit_port,
            resource_path: resource_path,
            is_ipv6: is_ipv6,
        };
    }

    fn isTls(): bool {
        return _.scheme == .Wss;
    }

    fn isDefaultPort(): bool {
        if (_.scheme == .Ws) return _.port == 80;
        return _.port == 443;
    }

    /// 格式化 RFC 6455 `Host` 头值（默认端口省略端口号，IPv6 自动加方括号）
    fn formatHostHeader(out: []u8): usize! {
        let pos: usize = 0;
        if (_.is_ipv6) appendBytes(out, &pos, "[")?;
        appendBytes(out, &pos, _.host)?;
        if (_.is_ipv6) appendBytes(out, &pos, "]")?;
        if (!_.isDefaultPort()) {
            appendBytes(out, &pos, ":")?;
            appendPort(out, &pos, _.port)?;
        }
        return pos;
    }

    /// 格式化规范化 `ws://` / `wss://` URL（支持无损 round-trip）
    fn formatCanonicalUrl(out: []u8): usize! {
        let pos: usize = 0;
        if (_.scheme == .Wss) {
            appendBytes(out, &pos, "wss://")?;
        } else {
            appendBytes(out, &pos, "ws://")?;
        }
        const host_len = _.formatHostHeader(out[pos..out.len])?;
        pos += host_len;
        if (_.resource_path.len > 0 && _.resource_path[0] == '?') {
            appendBytes(out, &pos, "/")?;
        }
        appendBytes(out, &pos, _.resource_path)?;
        return pos;
    }
}

/// 自定义升级请求头条目
pub struct WsClientHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// WebSocket 客户端升级配置（零动态堆分配）
pub struct WsClientUpgradeConfig {
    custom_headers: [WS_CLIENT_MAX_CUSTOM_HEADERS]WsClientHeader = [WsClientHeader{}; WS_CLIENT_MAX_CUSTOM_HEADERS],
    header_count: usize = 0,
    protocols: [WS_CLIENT_MAX_PROTOCOLS]([]const u8) = [""; WS_CLIENT_MAX_PROTOCOLS],
    protocol_count: usize = 0,
    origin: ([]const u8)? = null,
    enable_permessage_deflate: bool = false,

    fn addHeader(*_, name: []const u8, value: []const u8): ! {
        if (!isValidHttpToken(name) || hasControlOrCrlf(value)) {
            return err.InvalidHeader;
        }
        // 禁止覆盖 RFC 6455 核心保留握手头
        if (strEqIgnoreCase(name, "host") ||
            strEqIgnoreCase(name, "upgrade") ||
            strEqIgnoreCase(name, "connection") ||
            strEqIgnoreCase(name, "sec-websocket-key") ||
            strEqIgnoreCase(name, "sec-websocket-version"))
        {
            return err.InvalidHeader;
        }
        if (_.header_count >= WS_CLIENT_MAX_CUSTOM_HEADERS) {
            return err.TooManyHeaders;
        }
        _.custom_headers[_.header_count] = WsClientHeader{
            name: name,
            value: value,
        };
        _.header_count += 1;
    }

    fn addProtocol(*_, proto: []const u8): ! {
        if (!isValidHttpToken(proto)) return err.InvalidHeader;
        if (_.protocol_count >= WS_CLIENT_MAX_PROTOCOLS) {
            return err.TooManyProtocols;
        }
        _.protocols[_.protocol_count] = proto;
        _.protocol_count += 1;
    }

    fn containsProtocol(proto: []const u8): bool {
        let i: usize = 0;
        while (i < _.protocol_count) {
            if (strEq(_.protocols[i], proto)) return true;
            i += 1;
        }
        return false;
    }
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn findHeaderValue(raw_response: []const u8, target_name: []const u8): ([]const u8)? {
    // 跳过首行（状态行）
    let i: usize = 0;
    while (i + 1 < raw_response.len) {
        if (raw_response[i] == '\r' && raw_response[i + 1] == '\n') {
            i += 2;
            break;
        }
        i += 1;
    }
    if (i >= raw_response.len) return null;

    while (i < raw_response.len) {
        let line_end: usize = i;
        while (line_end + 1 < raw_response.len) {
            if (raw_response[line_end] == '\r' && raw_response[line_end + 1] == '\n') break;
            line_end += 1;
        }
        const line = raw_response[i..line_end];
        if (line.len == 0) break;

        let colon: usize? = null;
        let c: usize = 0;
        while (c < line.len) {
            if (line[c] == ':') {
                colon = c;
                break;
            }
            c += 1;
        }
        if (cp := colon) {
            const name = trimOws(line[0..cp]);
            if (strEqIgnoreCase(name, target_name)) {
                return trimOws(line[cp + 1..line.len]);
            }
        }

        if (line_end + 2 <= raw_response.len && raw_response[line_end] == '\r' && raw_response[line_end + 1] == '\n') {
            i = line_end + 2;
        } else {
            break;
        }
    }
    return null;
}

/// 纯 CP 零堆分配 WebSocket 客户端 HTTP/1.1 升级状态机
pub struct WsClientUpgradeMachine {
    url: WsClientUrl = WsClientUrl{},
    phase: WsUpgradePhase = .Idle,
    expected_accept_buf: [28]u8 = [0; 28],
    expected_accept_len: usize = 0,
    negotiated_protocol_buf: [64]u8 = [0; 64],
    negotiated_protocol_len: usize = 0,
    has_permessage_deflate: bool = false,

    static fn init(url: WsClientUrl): WsClientUpgradeMachine {
        return WsClientUpgradeMachine{
            url: url,
            phase: .Idle,
        };
    }

    /// 构造 RFC 6455 HTTP/1.1 升级请求报文并推进状态至 `AwaitingResponse`
    fn buildUpgradeRequest(
        *_,
        client_key: []const u8,
        cfg: WsClientUpgradeConfig,
        out: []u8,
    ): usize! {
        if (_.phase != .Idle) return err.InvalidState;
        if (client_key.len == 0 || hasControlOrCrlf(client_key)) return err.InvalidHeader;

        // 预先计算并缓存预期 28 字节 Sec-WebSocket-Accept 摘要
        const accept_slice = WebSocketHandshake.computeAcceptKey(client_key, _.expected_accept_buf[0..]) catch {
            return err.InvalidAcceptKey;
        };
        _.expected_accept_len = accept_slice.len;

        let pos: usize = 0;
        appendBytes(out, &pos, "GET ")?;
        if (_.url.resource_path.len > 0 && _.url.resource_path[0] == '?') {
            appendBytes(out, &pos, "/")?;
        }
        appendBytes(out, &pos, _.url.resource_path)?;
        appendBytes(out, &pos, " HTTP/1.1\r\nHost: ")?;
        const host_len = _.url.formatHostHeader(out[pos..out.len])?;
        pos += host_len;
        appendBytes(out, &pos, "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ")?;
        appendBytes(out, &pos, client_key)?;
        appendBytes(out, &pos, "\r\nSec-WebSocket-Version: 13\r\n")?;

        if (orig := cfg.origin) {
            if (hasControlOrCrlf(orig)) return err.InvalidHeader;
            appendBytes(out, &pos, "Origin: ")?;
            appendBytes(out, &pos, orig)?;
            appendBytes(out, &pos, "\r\n")?;
        }

        if (cfg.protocol_count > 0) {
            appendBytes(out, &pos, "Sec-WebSocket-Protocol: ")?;
            let p: usize = 0;
            while (p < cfg.protocol_count) {
                if (p > 0) appendBytes(out, &pos, ", ")?;
                appendBytes(out, &pos, cfg.protocols[p])?;
                p += 1;
            }
            appendBytes(out, &pos, "\r\n")?;
        }

        if (cfg.enable_permessage_deflate) {
            appendBytes(out, &pos, "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n")?;
        }

        let h: usize = 0;
        while (h < cfg.header_count) {
            appendBytes(out, &pos, cfg.custom_headers[h].name)?;
            appendBytes(out, &pos, ": ")?;
            appendBytes(out, &pos, cfg.custom_headers[h].value)?;
            appendBytes(out, &pos, "\r\n")?;
            h += 1;
        }

        appendBytes(out, &pos, "\r\n")?;
        _.phase = .AwaitingResponse;
        return pos;
    }

    /// 严格校验服务端 `HTTP/1.1 101 Switching Protocols` 升级响应
    fn verifyUpgradeResponse(
        *_,
        raw_response: []const u8,
        cfg: WsClientUpgradeConfig,
    ): ! {
        if (_.phase != .AwaitingResponse) return err.InvalidState;

        // 1. 校验状态行以 "HTTP/1.1 101" 开头
        if (raw_response.len < 12 || !strEq(raw_response[0..12], "HTTP/1.1 101")) {
            _.phase = .Failed;
            return err.UpgradeRejected;
        }

        // 2. 校验 Upgrade: websocket 与 Connection: Upgrade
        const upg = findHeaderValue(raw_response, "Upgrade") orelse {
            _.phase = .Failed;
            return err.UpgradeRejected;
        };
        if (!strEqIgnoreCase(upg, "websocket")) {
            _.phase = .Failed;
            return err.UpgradeRejected;
        }

        const conn = findHeaderValue(raw_response, "Connection") orelse {
            _.phase = .Failed;
            return err.UpgradeRejected;
        };
        if (!containsIgnoreCase(conn, "upgrade")) {
            _.phase = .Failed;
            return err.UpgradeRejected;
        }

        // 3. 严格比对 Sec-WebSocket-Accept
        const accept = findHeaderValue(raw_response, "Sec-WebSocket-Accept") orelse {
            _.phase = .Failed;
            return err.InvalidAcceptKey;
        };
        if (!strEq(accept, _.expected_accept_buf[0.._.expected_accept_len])) {
            _.phase = .Failed;
            return err.InvalidAcceptKey;
        }

        // 4. 校验子协议协商合法性
        _.negotiated_protocol_len = 0;
        if (proto := findHeaderValue(raw_response, "Sec-WebSocket-Protocol")) {
            if (proto.len > 0) {
                if (cfg.protocol_count == 0 || !cfg.containsProtocol(proto)) {
                    _.phase = .Failed;
                    return err.ProtocolMismatch;
                }
                if (proto.len > 64) {
                    _.phase = .Failed;
                    return err.BufferTooSmall;
                }
                let k: usize = 0;
                while (k < proto.len) {
                    _.negotiated_protocol_buf[k] = proto[k];
                    k += 1;
                }
                _.negotiated_protocol_len = proto.len;
            }
        }

        // 5. 检测 permessage-deflate 扩展协商结果
        _.has_permessage_deflate = false;
        if (ext := findHeaderValue(raw_response, "Sec-WebSocket-Extensions")) {
            if (cfg.enable_permessage_deflate && containsIgnoreCase(ext, "permessage-deflate")) {
                _.has_permessage_deflate = true;
            }
        }

        _.phase = .Upgraded;
    }

    fn negotiatedProtocol(): []const u8 {
        return _.negotiated_protocol_buf[0.._.negotiated_protocol_len];
    }
}
