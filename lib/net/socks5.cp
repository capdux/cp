// Source: cp/lib/net/socks5.cp (RFC 1928 / RFC 1929)
// lib/net/socks5.cp
// Zero-dependency SOCKS5 (RFC 1928 / RFC 1929) & HTTP CONNECT Wire Codec

import { Ipv4Addr } from "./address";
import { IpAddrCodec } from "./cidr";

pub enum Socks5AddrType: u8 {
    Ipv4 = 0x01,
    Domain = 0x03,
    Ipv6 = 0x04,
}

pub enum Socks5AuthMethod: u8 {
    NoAuth = 0x00,
    GssApi = 0x01,
    UserPass = 0x02,
    NoAcceptable = 0xFF,

    static fn fromByte(b: u8): Socks5AuthMethod? {
        switch (b) {
            0x00 => return Socks5AuthMethod.NoAuth,
            0x01 => return Socks5AuthMethod.GssApi,
            0x02 => return Socks5AuthMethod.UserPass,
            0xFF => return Socks5AuthMethod.NoAcceptable,
            else => return null,
        }
    }
}

pub enum Socks5ReplyCode: u8 {
    Succeeded = 0x00,
    GeneralFailure = 0x01,
    ConnectionNotAllowed = 0x02,
    NetworkUnreachable = 0x03,
    HostUnreachable = 0x04,
    ConnectionRefused = 0x05,
    TtlExpired = 0x06,
    CommandNotSupported = 0x07,
    AddressTypeNotSupported = 0x08,

    static fn fromByte(b: u8): Socks5ReplyCode? {
        switch (b) {
            0x00 => return Socks5ReplyCode.Succeeded,
            0x01 => return Socks5ReplyCode.GeneralFailure,
            0x02 => return Socks5ReplyCode.ConnectionNotAllowed,
            0x03 => return Socks5ReplyCode.NetworkUnreachable,
            0x04 => return Socks5ReplyCode.HostUnreachable,
            0x05 => return Socks5ReplyCode.ConnectionRefused,
            0x06 => return Socks5ReplyCode.TtlExpired,
            0x07 => return Socks5ReplyCode.CommandNotSupported,
            0x08 => return Socks5ReplyCode.AddressTypeNotSupported,
            else => return null,
        }
    }
}

pub struct Socks5MethodSelection {
    version: u8 = 0x05;
    method: Socks5AuthMethod = Socks5AuthMethod.NoAcceptable;
}

pub struct Socks5ConnectReply {
    reply: Socks5ReplyCode = Socks5ReplyCode.GeneralFailure;
    addr_type: Socks5AddrType = Socks5AddrType.Ipv4;
    ipv4: [4]u8 = [0, 0, 0, 0];
    ipv6: [16]u8 = [0; 16];
    domain_buf: [256]u8 = [0; 256];
    domain_len: usize = 0;
    port: u16 = 0;
    total_len: usize = 0;

    fn succeeded(): bool {
        return _.reply == Socks5ReplyCode.Succeeded;
    }

    fn domain(): []const u8 {
        return _.domain_buf[0.._.domain_len];
    }
}

pub struct Socks5Codec {
    /// Encodes a SOCKS5 client greeting with the specified authentication methods (RFC 1928 §3).
    pub static fn encodeGreeting(out: []u8, methods: []const Socks5AuthMethod): usize {
        if (methods.len == 0 || methods.len > 255) {
            return 0;
        }
        const total: usize = methods.len + 2;
        if (out.len < total) {
            return 0;
        }
        out[0] = 0x05;
        out[1] = methods.len as u8;
        let i: usize = 0;
        while (i < methods.len) {
            out[2 + i] = methods[i] as u8;
            i += 1;
        }
        return total;
    }

    /// Decodes a 2-byte SOCKS5 server method selection reply (`[VER=0x05, METHOD]`).
    pub static fn decodeMethodSelection(buf: []const u8): Socks5MethodSelection? {
        if (buf.len < 2 || buf[0] != 0x05) {
            return null;
        }
        const m = Socks5AuthMethod.fromByte(buf[1]) ?? return null;
        return Socks5MethodSelection {
            version: buf[0],
            method: m,
        };
    }

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

    /// Encodes an RFC 1929 username/password subnegotiation packet (`[0x01, ULEN, USER, PLEN, PASS]`).
    pub static fn encodeUserPassAuth(out: []u8, username: []const u8, password: []const u8): usize {
        if (username.len == 0 || username.len > 255 || password.len > 255) {
            return 0;
        }
        const total: usize = username.len + password.len + 3;
        if (out.len < total) {
            return 0;
        }
        out[0] = 0x01; // RFC 1929 subnegotiation version
        out[1] = username.len as u8;
        let i: usize = 0;
        while (i < username.len) {
            out[2 + i] = username[i];
            i += 1;
        }
        const plen_pos = 2 + username.len;
        out[plen_pos] = password.len as u8;
        i = 0;
        while (i < password.len) {
            out[plen_pos + 1 + i] = password[i];
            i += 1;
        }
        return total;
    }

    /// Decodes an RFC 1929 username/password reply (`[0x01, STATUS]`: `true` if STATUS == 0x00).
    pub static fn decodeUserPassReply(buf: []const u8): bool? {
        if (buf.len < 2 || buf[0] != 0x01) {
            return null;
        }
        return buf[1] == 0x00;
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

    /// Encodes a 22-byte SOCKS5 CONNECT request to an IPv6 address and port.
    pub static fn encodeConnectIpv6(out: []u8, ip: [16]u8, port: u16): usize {
        if (out.len < 22) {
            return 0;
        }
        out[0] = 0x05; // VER = 5
        out[1] = 0x01; // CMD = CONNECT
        out[2] = 0x00; // RSV = 0
        out[3] = 0x04; // ATYP = IPv6
        let i: usize = 0;
        while (i < 16) {
            out[4 + i] = ip[i];
            i += 1;
        }
        out[20] = ((port >> 8) & 0xFF) as u8;
        out[21] = (port & 0xFF) as u8;
        return 22;
    }

    /// Encodes a SOCKS5 CONNECT request to a domain name and port.
    pub static fn encodeConnectDomain(out: []u8, domain: []const u8, port: u16): usize {
        const total: usize = domain.len + 7;
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

    /// Encodes a SOCKS5 CONNECT request, automatically detecting IPv4 (`0x01`), IPv6 (`0x04`), or Domain (`0x03`).
    pub static fn encodeConnectRequest(out: []u8, host: []const u8, port: u16): usize {
        if (host.len == 0) {
            return 0;
        }
        if (v4 := Ipv4Addr.parse(host)) {
            return Socks5Codec.encodeConnectIpv4(out, v4.octets, port);
        }
        let v6_host = host;
        if (host.len >= 2 && host[0] == '[' && host[host.len - 1] == ']') {
            v6_host = host[1..host.len - 1];
        }
        if (v6 := IpAddrCodec.parseV6(v6_host)) {
            return Socks5Codec.encodeConnectIpv6(out, IpAddrCodec.v6ToBytes(v6), port);
        }
        return Socks5Codec.encodeConnectDomain(out, host, port);
    }

    /// Decodes a SOCKS5 CONNECT reply, extracting status code, bound address (IPv4/Domain/IPv6), and bound port.
    pub static fn decodeConnectReply(buf: []const u8): Socks5ConnectReply? {
        if (buf.len < 4 || buf[0] != 0x05) {
            return null;
        }
        const rep = Socks5ReplyCode.fromByte(buf[1]) ?? return null;
        let res = Socks5ConnectReply {};
        res.reply = rep;

        const atyp = buf[3];
        if (atyp == 0x01) {
            if (buf.len < 10) { return null; }
            res.addr_type = Socks5AddrType.Ipv4;
            res.ipv4[0] = buf[4];
            res.ipv4[1] = buf[5];
            res.ipv4[2] = buf[6];
            res.ipv4[3] = buf[7];
            res.port = ((buf[8] as u16) << 8) | (buf[9] as u16);
            res.total_len = 10;
            return res;
        }
        if (atyp == 0x03) {
            if (buf.len < 5) { return null; }
            const dlen = buf[4] as usize;
            const total: usize = dlen + 7;
            if (buf.len < total) { return null; }
            res.addr_type = Socks5AddrType.Domain;
            res.domain_len = dlen;
            let i: usize = 0;
            while (i < dlen) {
                res.domain_buf[i] = buf[5 + i];
                i += 1;
            }
            res.port = ((buf[5 + dlen] as u16) << 8) | (buf[6 + dlen] as u16);
            res.total_len = total;
            return res;
        }
        if (atyp == 0x04) {
            if (buf.len < 22) { return null; }
            res.addr_type = Socks5AddrType.Ipv6;
            let i: usize = 0;
            while (i < 16) {
                res.ipv6[i] = buf[4 + i];
                i += 1;
            }
            res.port = ((buf[20] as u16) << 8) | (buf[21] as u16);
            res.total_len = 22;
            return res;
        }
        return null;
    }

    /// Verifies a SOCKS5 CONNECT reply (`VER == 0x05` and `REP == 0x00` succeeded).
    pub static fn verifyConnectReply(reply_buf: []const u8): bool {
        return reply_buf.len >= 4 && reply_buf[0] == 0x05 && reply_buf[1] == 0x00;
    }
}

pub struct HttpConnectCodec {
    static fn writeU16Dec(out: []u8, pos: *usize, val: u16): bool {
        const p = pos.*;
        if (val == 0) {
            if (p + 1 > out.len) { return false; }
            out[p] = '0';
            pos.* = p + 1;
            return true;
        }
        let tmp: [8]u8 = [0; 8];
        let len: usize = 0;
        let v: u32 = val as u32;
        while (v > 0) {
            tmp[len] = (48 as u8) + ((v % 10) as u8);
            len += 1;
            v /= 10;
        }
        if (p + len > out.len) { return false; }
        let i: usize = 0;
        while (i < len) {
            out[p + i] = tmp[len - 1 - i];
            i += 1;
        }
        pos.* = p + len;
        return true;
    }

    static fn appendSlice(out: []u8, pos: *usize, s: []const u8): bool {
        const p = pos.*;
        if (p + s.len > out.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[p + i] = s[i];
            i += 1;
        }
        pos.* = p + s.len;
        return true;
    }

    static fn containsColon(s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == ':') { return true; }
            i += 1;
        }
        return false;
    }

    static fn appendHostPort(out: []u8, pos: *usize, host: []const u8, port: u16): bool {
        const needs_brackets = HttpConnectCodec.containsColon(host) && !(host.len >= 2 && host[0] == '[' && host[host.len - 1] == ']');
        if (needs_brackets) {
            if (!HttpConnectCodec.appendSlice(out, pos, "[")) { return false; }
        }
        if (!HttpConnectCodec.appendSlice(out, pos, host)) { return false; }
        if (needs_brackets) {
            if (!HttpConnectCodec.appendSlice(out, pos, "]")) { return false; }
        }
        if (!HttpConnectCodec.appendSlice(out, pos, ":")) { return false; }
        return HttpConnectCodec.writeU16Dec(out, pos, port);
    }

    /// Encodes an RFC 9110 / RFC 7231 HTTP `CONNECT host:port HTTP/1.1` request with optional `Proxy-Authorization`.
    pub static fn encodeRequest(out: []u8, host: []const u8, port: u16, proxy_auth_header: ([]const u8)?): usize {
        if (host.len == 0 || port == 0) {
            return 0;
        }
        let pos: usize = 0;
        if (!HttpConnectCodec.appendSlice(out, *pos, "CONNECT ")) { return 0; }
        if (!HttpConnectCodec.appendHostPort(out, *pos, host, port)) { return 0; }
        if (!HttpConnectCodec.appendSlice(out, *pos, " HTTP/1.1\r\nHost: ")) { return 0; }
        if (!HttpConnectCodec.appendHostPort(out, *pos, host, port)) { return 0; }
        if (!HttpConnectCodec.appendSlice(out, *pos, "\r\n")) { return 0; }

        if (auth := proxy_auth_header) {
            if (auth.len > 0) {
                if (!HttpConnectCodec.appendSlice(out, *pos, "Proxy-Authorization: ")) { return 0; }
                if (!HttpConnectCodec.appendSlice(out, *pos, auth)) { return 0; }
                if (!HttpConnectCodec.appendSlice(out, *pos, "\r\n")) { return 0; }
            }
        }

        if (!HttpConnectCodec.appendSlice(out, *pos, "Proxy-Connection: keep-alive\r\n\r\n")) { return 0; }
        return pos;
    }

    /// Parses the 3-digit HTTP status code from an HTTP CONNECT proxy response status line.
    pub static fn parseResponseStatus(buf: []const u8): u16? {
        if (buf.len < 12) {
            return null;
        }
        if (!std.mem.startsWith(u8, buf, "HTTP/1.")) {
            return null;
        }
        if (buf[7] != '0' && buf[7] != '1') {
            return null;
        }
        if (buf[8] != ' ') {
            return null;
        }
        const d0 = buf[9];
        const d1 = buf[10];
        const d2 = buf[11];
        if (d0 < '0' || d0 > '9' || d1 < '0' || d1 > '9' || d2 < '0' || d2 > '9') {
            return null;
        }
        if (buf.len > 12) {
            const next = buf[12];
            if (next != ' ' && next != '\r' && next != '\n') {
                return null;
            }
        }
        const code = (((d0 - '0') as u16) * 100) + (((d1 - '0') as u16) * 10) + ((d2 - '0') as u16);
        if (code < 100 || code > 599) {
            return null;
        }
        return code;
    }
}
