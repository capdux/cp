// Source: Bun src/http/ProxyTunnel.cp
// lib/net/proxy.cp
// HTTP CONNECT Proxy Tunnel & SOCKS5 Wire Handshake Codec
// Absorbed from Bun src/http/ProxyTunnel.cp

const std = @import("std");

pub struct HttpConnectTunnel {
    pub static fn writeU16Dec(out: []u8, val: u16): usize {
        if (val == 0) {
            if (out.len < 1) { return 0; }
            out[0] = '0';
            return 1;
        }
        let tmp: [8]u8 = [0; 8];
        let len: usize = 0;
        let v: u32 = val as u32;
        while (v > 0) {
            tmp[len] = (48 as u8) + ((v % 10) as u8);
            len += 1;
            v /= 10;
        }
        if (out.len < len) { return 0; }
        let i: usize = 0;
        while (i < len) {
            out[i] = tmp[len - 1 - i];
            i += 1;
        }
        return len;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Builds an RFC 7231 HTTP `CONNECT host:port HTTP/1.1` proxy tunnel request into `out`.
    pub static fn buildConnectRequest(out: []u8, target_host: []const u8, target_port: u16): usize {
        let port_buf: [8]u8 = [0; 8];
        const port_len = HttpConnectTunnel.writeU16Dec(port_buf[0..8], target_port);
        if (port_len == 0) { return 0; }
        const port_str = port_buf[0..port_len];

        let pos: usize = 0;
        pos = HttpConnectTunnel.appendSlice(out, pos, "CONNECT ");
        pos = HttpConnectTunnel.appendSlice(out, pos, target_host);
        pos = HttpConnectTunnel.appendSlice(out, pos, ":");
        pos = HttpConnectTunnel.appendSlice(out, pos, port_str);
        pos = HttpConnectTunnel.appendSlice(out, pos, " HTTP/1.1\r\nHost: ");
        pos = HttpConnectTunnel.appendSlice(out, pos, target_host);
        pos = HttpConnectTunnel.appendSlice(out, pos, ":");
        pos = HttpConnectTunnel.appendSlice(out, pos, port_str);
        pos = HttpConnectTunnel.appendSlice(out, pos, "\r\nProxy-Connection: keep-alive\r\n\r\n");
        return pos;
    }

    /// Parses the HTTP status code from a proxy's response (e.g. `"HTTP/1.1 200 Connection Established\r\n\r\n"` -> `200`).
    pub static fn parseConnectResponse(resp: []const u8): u16? {
        if (resp.len < 12) {
            return null;
        }
        if (!std.mem.startsWith(u8, resp, "HTTP/1.")) {
            return null;
        }
        if (resp[8] != ' ') {
            return null;
        }
        const d0 = resp[9];
        const d1 = resp[10];
        const d2 = resp[11];
        if (d0 < '0' || d0 > '9' || d1 < '0' || d1 > '9' || d2 < '0' || d2 > '9') {
            return null;
        }
        return (((d0 - '0') as u16) * 100) + (((d1 - '0') as u16) * 10) + ((d2 - '0') as u16);
    }

    pub static fn isTunnelEstablished(resp: []const u8): bool {
        if (code := HttpConnectTunnel.parseConnectResponse(resp)) {
            return code == 200;
        }
        return false;
    }
}

pub enum Socks5AddrType: u8 {
    Ipv4 = 0x01,
    Domain = 0x03,
    Ipv6 = 0x04,
}

pub struct Socks5Codec {
    /// Encodes a 3-byte SOCKS5 no-authentication greeting (`[0x05, 0x01, 0x00]`).
    pub static fn encodeGreetingNoAuth(out: []u8): usize {
        if (out.len < 3) {
            return 0;
        }
        out[0] = 0x05;
        out[1] = 0x01;
        out[2] = 0x00;
        return 3;
    }

    /// Verifies a 2-byte SOCKS5 server greeting reply (`[0x05, 0x00]`).
    pub static fn verifyGreetingReply(reply_buf: []const u8): bool {
        return reply_buf.len >= 2 && reply_buf[0] == 0x05 && reply_buf[1] == 0x00;
    }

    /// Encodes a 10-byte SOCKS5 CONNECT request to an IPv4 address and port.
    pub static fn encodeConnectIpv4(out: []u8, ip: [4]u8, port: u16): usize {
        if (out.len < 10) {
            return 0;
        }
        out[0] = 0x05; // VER = 5
        out[1] = 0x01; // CMD = CONNECT
        out[2] = 0x00; // RSV = 0
        out[3] = 0x01; // ATYP = IPv4
        out[4] = ip[0];
        out[5] = ip[1];
        out[6] = ip[2];
        out[7] = ip[3];
        out[8] = ((port >> 8) & 0xFF) as u8;
        out[9] = (port & 0xFF) as u8;
        return 10;
    }

    /// Encodes a SOCKS5 CONNECT request to a domain name and port.
    pub static fn encodeConnectDomain(out: []u8, domain: []const u8, port: u16): usize {
        const total = 4 + 1 + domain.len + 2;
        if (domain.len == 0 || domain.len > 255 || out.len < total) {
            return 0;
        }
        out[0] = 0x05; // VER = 5
        out[1] = 0x01; // CMD = CONNECT
        out[2] = 0x00; // RSV = 0
        out[3] = 0x03; // ATYP = DOMAINNAME
        out[4] = domain.len as u8;
        let i: usize = 0;
        while (i < domain.len) {
            out[5 + i] = domain[i];
            i += 1;
        }
        out[5 + domain.len] = ((port >> 8) & 0xFF) as u8;
        out[6 + domain.len] = (port & 0xFF) as u8;
        return total;
    }

    /// Verifies a SOCKS5 CONNECT reply (`VER == 0x05` and `REP == 0x00` succeeded).
    pub static fn verifyConnectReply(reply_buf: []const u8): bool {
        return reply_buf.len >= 4 && reply_buf[0] == 0x05 && reply_buf[1] == 0x00;
    }
}
