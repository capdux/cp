// @cap/https-proxy-agent — Zero-heap HTTP/HTTPS proxy URL parser & CONNECT builder in pure CP
// Clean-room MIT implementation of npm `https-proxy-agent` (#60, 386.9M/wk) & `http-proxy-agent` (#220).

import { startsWith, findByte } from "cap:ascii";
import { writeStr, writeU64, parseU16 } from "cap:fmt";

pub struct ProxyEndpoint {
    pub scheme: string,   // "http" or "https"
    pub host: string,
    pub port: u16,
    pub username: string,
    pub password: string,
    pub has_auth: bool,
    pub is_tls: bool,
    pub valid: bool,
}

fn hpaParsePort(s: string): u16? {
    if (s.len == 0 || s[0] == 43) { return null; }
    const v = parseU16(s);
    if (v == null || (v ?? 0) == 0) { return null; }
    return v;
}

/// Parses a proxy URL such as `http://user:pass@proxy.local:8080` or `https://[::1]:3128`.
pub fn parseProxyUrl(url: string): ProxyEndpoint {
    const inv = ProxyEndpoint {
        scheme: "", host: "", port: 0, username: "", password: "",
        has_auth: false, is_tls: false, valid: false,
    };
    let scheme = "http";
    let rest = url;
    let is_tls = false;
    let default_port: u16 = 80;

    if (startsWith(url, "https://")) {
        scheme = "https";
        rest = url[8..url.len];
        is_tls = true;
        default_port = 443;
    } else if (startsWith(url, "http://")) {
        scheme = "http";
        rest = url[7..url.len];
        is_tls = false;
        default_port = 80;
    } else {
        return inv;
    }

    // Strip trailing path if any
    const slash = findByte(rest, 47, 0);
    let authority = rest;
    if (slash != null) {
        authority = rest[0..(slash ?? rest.len)];
    }
    if (authority.len == 0) {
        return inv;
    }

    // Optional user:pass@
    let username = "";
    let password = "";
    let has_auth = false;
    let hostport = authority;
    const at_opt = findByte(authority, 64, 0);
    if (at_opt != null) {
        const at = at_opt ?? 0;
        const userinfo = authority[0..at];
        hostport = authority[(at + 1)..authority.len];
        has_auth = true;
        const colon = findByte(userinfo, 58, 0);
        if (colon != null) {
            const c = colon ?? 0;
            username = userinfo[0..c];
            password = userinfo[(c + 1)..userinfo.len];
        } else {
            username = userinfo;
        }
    }

    if (hostport.len == 0) {
        return inv;
    }

    let host = hostport;
    let port = default_port;
    if (hostport[0] == 91) { // '[' IPv6 literal
        const close_b = findByte(hostport, 93, 1);
        if (close_b == null) { return inv; }
        const cb = close_b ?? 0;
        host = hostport[1..cb];
        if (cb + 1 < hostport.len) {
            if (hostport[cb + 1] != 58) { return inv; }
            const p_opt = hpaParsePort(hostport[(cb + 2)..hostport.len]);
            if (p_opt == null) { return inv; }
            port = p_opt ?? default_port;
        }
    } else {
        const colon = findByte(hostport, 58, 0);
        if (colon != null) {
            const c = colon ?? 0;
            host = hostport[0..c];
            const p_opt = hpaParsePort(hostport[(c + 1)..hostport.len]);
            if (p_opt == null) { return inv; }
            port = p_opt ?? default_port;
        }
    }

    if (host.len == 0) {
        return inv;
    }
    return ProxyEndpoint {
        scheme: scheme,
        host: host,
        port: port,
        username: username,
        password: password,
        has_auth: has_auth,
        is_tls: is_tls,
        valid: true,
    };
}

fn hpaWriteBase64(input: []const u8, out: []u8, pos: usize): usize {
    const tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let p = pos;
    let i: usize = 0;
    while (i < input.len) {
        const b0: u32 = input[i] as u32;
        const has1 = i + 1 < input.len;
        const has2 = i + 2 < input.len;
        const b1: u32 = if (has1) (input[i + 1] as u32) else 0;
        const b2: u32 = if (has2) (input[i + 2] as u32) else 0;
        const triple = (b0 << 16) | (b1 << 8) | b2;
        if (p + 3 < out.len) {
            out[p] = tbl[((triple >> 18) & 0x3F) as usize];
            out[p + 1] = tbl[((triple >> 12) & 0x3F) as usize];
            out[p + 2] = if (has1) tbl[((triple >> 6) & 0x3F) as usize] else 61;
            out[p + 3] = if (has2) tbl[(triple & 0x3F) as usize] else 61;
            p += 4;
        }
        i += 3;
    }
    return p;
}

/// Builds an RFC 9110 `CONNECT target_host:target_port HTTP/1.1` tunnel handshake request,
/// automatically appending `Proxy-Authorization: Basic <base64>` when `proxy.has_auth` is true.
pub fn buildConnectRequest(
    target_host: string,
    target_port: u16,
    proxy: ProxyEndpoint,
    out: []u8,
): usize {
    let p: usize = 0;
    p = writeStr("CONNECT ", out, p);
    p = writeStr(target_host, out, p);
    p = writeStr(":", out, p);
    p = writeU64(target_port as u64, out, p);
    p = writeStr(" HTTP/1.1\r\nHost: ", out, p);
    p = writeStr(target_host, out, p);
    p = writeStr(":", out, p);
    p = writeU64(target_port as u64, out, p);
    p = writeStr("\r\n", out, p);

    if (proxy.has_auth) {
        let cred: [256]u8 = undefined;
        let clen = writeStr(proxy.username, cred[0..256], 0);
        clen = writeStr(":", cred[0..256], clen);
        clen = writeStr(proxy.password, cred[0..256], clen);
        p = writeStr("Proxy-Authorization: Basic ", out, p);
        p = hpaWriteBase64(cred[0..clen], out, p);
        p = writeStr("\r\n", out, p);
    }

    p = writeStr("Connection: keep-alive\r\n\r\n", out, p);
    return p;
}

export {
    ProxyEndpoint,
    parseProxyUrl,
    buildConnectRequest,
};
