// Source: cp/lib/socks5/socks5.cp (RFC 1928 & RFC 1929 SOCKS5 Wire Protocol Codec)

const std = @import("std");

pub const SOCKS_VERSION: u8 = 0x05;
pub const USERPASS_AUTH_VERSION: u8 = 0x01;
pub const SOCKS_RSV: u8 = 0x00;

/// Errors returned by `cap:socks5` wire protocol encoding and decoding.
pub err Socks5Error {
    InvalidVersion,
    UnexpectedEof,
    InvalidMethodCount,
    InvalidAddressType,
    InvalidCommand,
    InvalidReplyCode,
    DomainTooLong,
    CredentialsTooLong,
    TrailingBytes,
    OutOfMemory,
}

/// RFC 1928 Section 3 authentication methods.
pub enum AuthMethod: u8 {
    NoAuth = 0x00,
    Gssapi = 0x01,
    UserPass = 0x02,
    NoAcceptable = 0xFF,

    pub static fn fromByte(b: u8): AuthMethod? {
        switch (b) {
            0x00 => return AuthMethod.NoAuth,
            0x01 => return AuthMethod.Gssapi,
            0x02 => return AuthMethod.UserPass,
            0xFF => return AuthMethod.NoAcceptable,
            else => return null,
        }
    }
}

/// RFC 1928 Section 4 request commands.
pub enum SocksCommand: u8 {
    Connect = 0x01,
    Bind = 0x02,
    UdpAssociate = 0x03,

    pub static fn fromByte(b: u8): SocksCommand ! Socks5Error {
        switch (b) {
            0x01 => return SocksCommand.Connect,
            0x02 => return SocksCommand.Bind,
            0x03 => return SocksCommand.UdpAssociate,
            else => return Socks5Error.InvalidCommand,
        }
    }
}

/// RFC 1928 Section 4 address types.
pub enum AddressType: u8 {
    Ipv4 = 0x01,
    Domain = 0x03,
    Ipv6 = 0x04,

    pub static fn fromByte(b: u8): AddressType ! Socks5Error {
        switch (b) {
            0x01 => return AddressType.Ipv4,
            0x03 => return AddressType.Domain,
            0x04 => return AddressType.Ipv6,
            else => return Socks5Error.InvalidAddressType,
        }
    }
}

/// RFC 1928 Section 6 reply codes.
pub enum ReplyCode: u8 {
    Succeeded = 0x00,
    GeneralFailure = 0x01,
    ConnectionNotAllowed = 0x02,
    NetworkUnreachable = 0x03,
    HostUnreachable = 0x04,
    ConnectionRefused = 0x05,
    TtlExpired = 0x06,
    CommandNotSupported = 0x07,
    AddressTypeNotSupported = 0x08,

    pub static fn fromByte(b: u8): ReplyCode ! Socks5Error {
        switch (b) {
            0x00 => return ReplyCode.Succeeded,
            0x01 => return ReplyCode.GeneralFailure,
            0x02 => return ReplyCode.ConnectionNotAllowed,
            0x03 => return ReplyCode.NetworkUnreachable,
            0x04 => return ReplyCode.HostUnreachable,
            0x05 => return ReplyCode.ConnectionRefused,
            0x06 => return ReplyCode.TtlExpired,
            0x07 => return ReplyCode.CommandNotSupported,
            0x08 => return ReplyCode.AddressTypeNotSupported,
            else => return Socks5Error.InvalidReplyCode,
        }
    }
}

/// Tagged union representing a SOCKS5 target or bound address.
pub enum SocksAddr {
    ipv4: [4]u8,
    domain: []const u8,
    ipv6: [16]u8,
}

/// Decoded RFC 1929 username/password credentials (zero-copy slices into input buffer).
pub struct UserPassCredentials {
    username: []const u8;
    password: []const u8;
}

/// RFC 1928 Section 4 SOCKS5 client request.
pub struct SocksRequest {
    cmd: SocksCommand;
    addr: SocksAddr;
    port: u16;
}

/// RFC 1928 Section 6 SOCKS5 server reply.
pub struct SocksReply {
    reply: ReplyCode;
    addr: SocksAddr;
    port: u16;
}

/// RFC 1928 Section 7 SOCKS5 UDP associate datagram header + payload.
pub struct SocksUdpPacket {
    frag: u8;
    addr: SocksAddr;
    port: u16;
    payload: []const u8;
}

struct DecodedAddrPort {
    addr: SocksAddr;
    port: u16;
    next_pos: usize;
}

fn encodedAddrLen(addr: SocksAddr): usize ! Socks5Error {
    switch (addr) {
        .ipv4 => return 1 + 4,
        .domain d => {
            if (d.len == 0 || d.len > 255) {
                return Socks5Error.DomainTooLong;
            }
            return 1 + 1 + d.len;
        },
        .ipv6 => return 1 + 16,
    }
}

fn writeAddrAndPort(buf: []u8, start: usize, addr: SocksAddr, port: u16): usize {
    let pos: usize = start;
    switch (addr) {
        .ipv4 v4 => {
            buf[pos] = AddressType.Ipv4 as u8;
            pos += 1;
            let i: usize = 0;
            while (i < 4) : (i += 1) {
                buf[pos + i] = v4[i];
            }
            pos += 4;
        },
        .domain d => {
            buf[pos] = AddressType.Domain as u8;
            buf[pos + 1] = d.len as u8;
            pos += 2;
            let i: usize = 0;
            while (i < d.len) : (i += 1) {
                buf[pos + i] = d[i];
            }
            pos += d.len;
        },
        .ipv6 v6 => {
            buf[pos] = AddressType.Ipv6 as u8;
            pos += 1;
            let i: usize = 0;
            while (i < 16) : (i += 1) {
                buf[pos + i] = v6[i];
            }
            pos += 16;
        },
    }
    buf[pos] = ((port >> 8) & 0xFF) as u8;
    buf[pos + 1] = (port & 0xFF) as u8;
    return pos + 2;
}

fn readPortBe(bytes: []const u8, pos: usize): u16 {
    const hi: u16 = bytes[pos] as u16;
    const lo: u16 = bytes[pos + 1] as u16;
    return (hi << 8) | lo;
}

fn decodeAddrAndPort(bytes: []const u8, atyp_offset: usize): DecodedAddrPort ! Socks5Error {
    if (bytes.len <= atyp_offset) {
        return Socks5Error.UnexpectedEof;
    }
    const atyp = AddressType.fromByte(bytes[atyp_offset])?;
    let pos: usize = atyp_offset + 1;

    switch (atyp) {
        .Ipv4 => {
            if (bytes.len < pos + 4 + 2) {
                return Socks5Error.UnexpectedEof;
            }
            let v4: [4]u8 = [0, 0, 0, 0];
            let i: usize = 0;
            while (i < 4) : (i += 1) {
                v4[i] = bytes[pos + i];
            }
            pos += 4;
            return DecodedAddrPort{
                addr: SocksAddr.ipv4(v4),
                port: readPortBe(bytes, pos),
                next_pos: pos + 2,
            };
        },
        .Domain => {
            if (bytes.len < pos + 1) {
                return Socks5Error.UnexpectedEof;
            }
            const dlen: usize = bytes[pos] as usize;
            pos += 1;
            if (dlen == 0) {
                return Socks5Error.DomainTooLong;
            }
            if (bytes.len < pos + dlen + 2) {
                return Socks5Error.UnexpectedEof;
            }
            const dom = bytes[pos .. pos + dlen];
            pos += dlen;
            return DecodedAddrPort{
                addr: SocksAddr.domain(dom),
                port: readPortBe(bytes, pos),
                next_pos: pos + 2,
            };
        },
        .Ipv6 => {
            if (bytes.len < pos + 16 + 2) {
                return Socks5Error.UnexpectedEof;
            }
            let v6: [16]u8 = [0; 16];
            let i: usize = 0;
            while (i < 16) : (i += 1) {
                v6[i] = bytes[pos + i];
            }
            pos += 16;
            return DecodedAddrPort{
                addr: SocksAddr.ipv6(v6),
                port: readPortBe(bytes, pos),
                next_pos: pos + 2,
            };
        },
    }
}

/// Encodes a SOCKS5 client greeting: [VER=0x05, NMETHODS, METHODS...].
pub fn encodeGreeting(allocator: std.mem.Allocator, methods: []const AuthMethod): []u8 ! Socks5Error {
    if (methods.len == 0 || methods.len > 255) {
        return Socks5Error.InvalidMethodCount;
    }
    let out = allocator.alloc(u8, 2 + methods.len) catch return Socks5Error.OutOfMemory;
    out[0] = SOCKS_VERSION;
    out[1] = methods.len as u8;
    let i: usize = 0;
    while (i < methods.len) : (i += 1) {
        out[2 + i] = methods[i] as u8;
    }
    return out;
}

/// Decodes a SOCKS5 client greeting and returns the raw method byte slice.
pub fn decodeGreeting(bytes: []const u8): []const u8 ! Socks5Error {
    if (bytes.len < 2) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes[0] != SOCKS_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    const n: usize = bytes[1] as usize;
    if (n == 0) {
        return Socks5Error.InvalidMethodCount;
    }
    if (bytes.len < 2 + n) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes.len > 2 + n) {
        return Socks5Error.TrailingBytes;
    }
    return bytes[2 .. 2 + n];
}

/// Encodes a SOCKS5 server method selection: [VER=0x05, METHOD].
pub fn encodeMethodChoice(method: AuthMethod): [2]u8 {
    return [SOCKS_VERSION, method as u8];
}

/// Decodes a SOCKS5 server method selection.
pub fn decodeMethodChoice(bytes: []const u8): AuthMethod ! Socks5Error {
    if (bytes.len < 2) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes.len > 2) {
        return Socks5Error.TrailingBytes;
    }
    if (bytes[0] != SOCKS_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    const m = AuthMethod.fromByte(bytes[1]);
    if (m == null) {
        return Socks5Error.InvalidMethodCount;
    }
    return m.?;
}

/// Encodes an RFC 1929 username/password authentication request.
pub fn encodeUserPassAuth(allocator: std.mem.Allocator, user: []const u8, pass: []const u8): []u8 ! Socks5Error {
    if (user.len == 0 || user.len > 255 || pass.len == 0 || pass.len > 255) {
        return Socks5Error.CredentialsTooLong;
    }
    const total: usize = @as(usize, 3) + user.len + pass.len;
    let out = allocator.alloc(u8, total) catch return Socks5Error.OutOfMemory;
    out[0] = USERPASS_AUTH_VERSION;
    out[1] = user.len as u8;
    let i: usize = 0;
    while (i < user.len) : (i += 1) {
        out[2 + i] = user[i];
    }
    out[2 + user.len] = pass.len as u8;
    let j: usize = 0;
    while (j < pass.len) : (j += 1) {
        out[3 + user.len + j] = pass[j];
    }
    return out;
}

/// Decodes an RFC 1929 username/password authentication request.
pub fn decodeUserPassAuth(bytes: []const u8): UserPassCredentials ! Socks5Error {
    if (bytes.len < 2) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes[0] != USERPASS_AUTH_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    const ulen: usize = bytes[1] as usize;
    if (ulen == 0) {
        return Socks5Error.CredentialsTooLong;
    }
    if (bytes.len < 2 + ulen + 1) {
        return Socks5Error.UnexpectedEof;
    }
    const plen: usize = bytes[2 + ulen] as usize;
    if (plen == 0) {
        return Socks5Error.CredentialsTooLong;
    }
    const total: usize = @as(usize, 3) + ulen + plen;
    if (bytes.len < total) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes.len > total) {
        return Socks5Error.TrailingBytes;
    }
    return UserPassCredentials{
        username: bytes[2 .. 2 + ulen],
        password: bytes[3 + ulen .. total],
    };
}

/// Encodes an RFC 1929 authentication status response: [VER=0x01, STATUS].
pub fn encodeAuthStatus(status: u8): [2]u8 {
    return [USERPASS_AUTH_VERSION, status];
}

/// Decodes an RFC 1929 authentication status response and returns the status byte.
pub fn decodeAuthStatus(bytes: []const u8): u8 ! Socks5Error {
    if (bytes.len < 2) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes.len > 2) {
        return Socks5Error.TrailingBytes;
    }
    if (bytes[0] != USERPASS_AUTH_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    return bytes[1];
}

/// Encodes an RFC 1928 Section 4 SOCKS5 request.
pub fn encodeRequest(allocator: std.mem.Allocator, req: SocksRequest): []u8 ! Socks5Error {
    const addr_len = encodedAddrLen(req.addr)?;
    const total: usize = @as(usize, 3) + addr_len + 2;
    let out = allocator.alloc(u8, total) catch return Socks5Error.OutOfMemory;
    out[0] = SOCKS_VERSION;
    out[1] = req.cmd as u8;
    out[2] = SOCKS_RSV;
    * = writeAddrAndPort(out, 3, req.addr, req.port);
    return out;
}

/// Decodes an RFC 1928 Section 4 SOCKS5 request.
pub fn decodeRequest(bytes: []const u8): SocksRequest ! Socks5Error {
    if (bytes.len < 4) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes[0] != SOCKS_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    const cmd = SocksCommand.fromByte(bytes[1])?;
    const ap = decodeAddrAndPort(bytes, 3)?;
    if (ap.next_pos != bytes.len) {
        return Socks5Error.TrailingBytes;
    }
    return SocksRequest{
        cmd: cmd,
        addr: ap.addr,
        port: ap.port,
    };
}

/// Encodes an RFC 1928 Section 6 SOCKS5 reply.
pub fn encodeReply(allocator: std.mem.Allocator, rep: SocksReply): []u8 ! Socks5Error {
    const addr_len = encodedAddrLen(rep.addr)?;
    const total: usize = @as(usize, 3) + addr_len + 2;
    let out = allocator.alloc(u8, total) catch return Socks5Error.OutOfMemory;
    out[0] = SOCKS_VERSION;
    out[1] = rep.reply as u8;
    out[2] = SOCKS_RSV;
    * = writeAddrAndPort(out, 3, rep.addr, rep.port);
    return out;
}

/// Decodes an RFC 1928 Section 6 SOCKS5 reply.
pub fn decodeReply(bytes: []const u8): SocksReply ! Socks5Error {
    if (bytes.len < 4) {
        return Socks5Error.UnexpectedEof;
    }
    if (bytes[0] != SOCKS_VERSION) {
        return Socks5Error.InvalidVersion;
    }
    const reply = ReplyCode.fromByte(bytes[1])?;
    const ap = decodeAddrAndPort(bytes, 3)?;
    if (ap.next_pos != bytes.len) {
        return Socks5Error.TrailingBytes;
    }
    return SocksReply{
        reply: reply,
        addr: ap.addr,
        port: ap.port,
    };
}

/// Encodes an RFC 1928 Section 7 SOCKS5 UDP associate packet.
pub fn encodeUdpPacket(allocator: std.mem.Allocator, pkt: SocksUdpPacket): []u8 ! Socks5Error {
    const addr_len = encodedAddrLen(pkt.addr)?;
    const total: usize = @as(usize, 3) + addr_len + 2 + pkt.payload.len;
    let out = allocator.alloc(u8, total) catch return Socks5Error.OutOfMemory;
    out[0] = SOCKS_RSV;
    out[1] = SOCKS_RSV;
    out[2] = pkt.frag;
    const payload_start = writeAddrAndPort(out, 3, pkt.addr, pkt.port);
    let i: usize = 0;
    while (i < pkt.payload.len) : (i += 1) {
        out[payload_start + i] = pkt.payload[i];
    }
    return out;
}

/// Decodes an RFC 1928 Section 7 SOCKS5 UDP associate packet.
pub fn decodeUdpPacket(bytes: []const u8): SocksUdpPacket ! Socks5Error {
    if (bytes.len < 4) {
        return Socks5Error.UnexpectedEof;
    }
    const frag: u8 = bytes[2];
    const ap = decodeAddrAndPort(bytes, 3)?;
    return SocksUdpPacket{
        frag: frag,
        addr: ap.addr,
        port: ap.port,
        payload: bytes[ap.next_pos .. bytes.len],
    };
}
