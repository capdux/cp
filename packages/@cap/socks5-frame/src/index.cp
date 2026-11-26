// packages/@cap/socks5-frame/src/index.cp
// Zero-allocation RFC 1928 SOCKS5 & RFC 1929 Username/Password frame codec.
// MIT clean-room pure CP implementation.

pub const SOCKS_V5: u8 = 0x05;

pub const AUTH_NONE: u8 = 0x00;
pub const AUTH_USERPASS: u8 = 0x02;
pub const AUTH_NO_ACCEPTABLE: u8 = 0xFF;

pub const CMD_CONNECT: u8 = 0x01;
pub const CMD_BIND: u8 = 0x02;
pub const CMD_UDP_ASSOCIATE: u8 = 0x03;

pub const ATYP_IPV4: u8 = 0x01;
pub const ATYP_DOMAIN: u8 = 0x03;
pub const ATYP_IPV6: u8 = 0x04;

pub const S5_OK: i32 = 0;
pub const S5_INCOMPLETE: i32 = 1;
pub const S5_INVALID: i32 = -1;

/// Decoded RFC 1928 SOCKS5 request frame.
pub struct Socks5Request {
    pub cmd: u8,
    pub atyp: u8,
    pub ipv4: [4]u8,
    pub ipv6: [16]u8,
    pub domain: string,
    pub port: u16,
    pub consumed: usize,
}

/// Encode client SOCKS5 greeting `[0x05, nmethods, methods...]` into `out`.
/// Returns bytes written, or `0` on invalid method count or insufficient `out` buffer.
pub fn encodeGreeting(methods: []const u8, out: []u8): usize {
    if (methods.len == 0 || methods.len > 255) {
        return 0;
    }
    const total = 2 + methods.len;
    if (out.len < total) {
        return 0;
    }
    out[0] = SOCKS_V5;
    out[1] = methods.len as u8;
    let i: usize = 0;
    while (i < methods.len) {
        out[2 + i] = methods[i];
        i += 1;
    }
    return total;
}

/// Parse a client SOCKS5 greeting frame and select the first mutually supported auth method.
/// Writes selected method (or `AUTH_NO_ACCEPTABLE`) to `out_method.*`.
/// Returns `S5_OK` (0), `S5_INCOMPLETE` (1), or `S5_INVALID` (-1).
pub fn selectMethod(greeting: []const u8, supported: []const u8, out_method: *u8): i32 {
    if (greeting.len < 2) {
        return S5_INCOMPLETE;
    }
    if (greeting[0] != SOCKS_V5) {
        return S5_INVALID;
    }
    const nmethods = greeting[1] as usize;
    if (nmethods == 0) {
        return S5_INVALID;
    }
    if (greeting.len < 2 + nmethods) {
        return S5_INCOMPLETE;
    }

    let i: usize = 0;
    while (i < nmethods) {
        const offered = greeting[2 + i];
        let j: usize = 0;
        while (j < supported.len) {
            if (offered == supported[j]) {
                out_method.* = offered;
                return S5_OK;
            }
            j += 1;
        }
        i += 1;
    }

    out_method.* = AUTH_NO_ACCEPTABLE;
    return S5_OK;
}

/// Encode RFC 1929 Username/Password subnegotiation `[0x01, ulen, user, plen, pass]`.
/// Returns bytes written, or `0` on empty/>255 credential or insufficient `out` buffer.
pub fn encodeUserPassAuth(user: string, pass: string, out: []u8): usize {
    if (user.len == 0 || user.len > 255 || pass.len == 0 || pass.len > 255) {
        return 0;
    }
    const total = 3 + user.len + pass.len;
    if (out.len < total) {
        return 0;
    }
    out[0] = 0x01 as u8;
    out[1] = user.len as u8;
    let pos: usize = 2;
    let ui: usize = 0;
    while (ui < user.len) {
        out[pos] = user[ui];
        pos += 1;
        ui += 1;
    }
    out[pos] = pass.len as u8;
    pos += 1;
    let pi: usize = 0;
    while (pi < pass.len) {
        out[pos] = pass[pi];
        pos += 1;
        pi += 1;
    }
    return total;
}

/// Encode a SOCKS5 `CONNECT` request targeting a domain name and big-endian port.
/// Returns bytes written, or `0` on invalid host or insufficient `out` buffer.
pub fn encodeConnectDomain(host: string, port: u16, out: []u8): usize {
    if (host.len == 0 || host.len > 255) {
        return 0;
    }
    const total = 7 + host.len;
    if (out.len < total) {
        return 0;
    }
    out[0] = SOCKS_V5;
    out[1] = CMD_CONNECT;
    out[2] = 0x00 as u8;
    out[3] = ATYP_DOMAIN;
    out[4] = host.len as u8;
    let i: usize = 0;
    while (i < host.len) {
        out[5 + i] = host[i];
        i += 1;
    }
    out[5 + host.len] = ((port >> (8 as u4)) & (0xFF as u16)) as u8;
    out[6 + host.len] = (port & (0xFF as u16)) as u8;
    return total;
}

/// Encode a 10-byte SOCKS5 `CONNECT` request targeting an IPv4 address and big-endian port.
/// Returns `10` on success, or `0` if `out.len < 10`.
pub fn encodeConnectIpv4(ip: [4]u8, port: u16, out: []u8): usize {
    if (out.len < 10) {
        return 0;
    }
    out[0] = SOCKS_V5;
    out[1] = CMD_CONNECT;
    out[2] = 0x00 as u8;
    out[3] = ATYP_IPV4;
    out[4] = ip[0];
    out[5] = ip[1];
    out[6] = ip[2];
    out[7] = ip[3];
    out[8] = ((port >> (8 as u4)) & (0xFF as u16)) as u8;
    out[9] = (port & (0xFF as u16)) as u8;
    return 10;
}

/// Decode an RFC 1928 SOCKS5 request frame (IPv4, DOMAIN, or IPv6) with big-endian port.
/// Validates `ver == 0x05`, `rsv == 0x00`, and command/address type.
/// Returns `S5_OK` (0), `S5_INCOMPLETE` (1), or `S5_INVALID` (-1).
pub fn decodeRequest(data: []const u8, out: *Socks5Request): i32 {
    if (data.len < 4) {
        return S5_INCOMPLETE;
    }
    if (data[0] != SOCKS_V5) {
        return S5_INVALID;
    }
    const cmd = data[1];
    if (cmd != CMD_CONNECT && cmd != CMD_BIND && cmd != CMD_UDP_ASSOCIATE) {
        return S5_INVALID;
    }
    if (data[2] != (0x00 as u8)) {
        return S5_INVALID;
    }
    const atyp = data[3];

    let v4: [4]u8 = undefined;
    let idx4: usize = 0;
    while (idx4 < 4) {
        v4[idx4] = 0 as u8;
        idx4 += 1;
    }
    let v6: [16]u8 = undefined;
    let idx6: usize = 0;
    while (idx6 < 16) {
        v6[idx6] = 0 as u8;
        idx6 += 1;
    }

    if (atyp == ATYP_IPV4) {
        if (data.len < 10) {
            return S5_INCOMPLETE;
        }
        v4[0] = data[4];
        v4[1] = data[5];
        v4[2] = data[6];
        v4[3] = data[7];
        const port = ((data[8] as u16) << (8 as u4)) | (data[9] as u16);
        out.* = Socks5Request{
            cmd: cmd,
            atyp: atyp,
            ipv4: v4,
            ipv6: v6,
            domain: "",
            port: port,
            consumed: 10,
        };
        return S5_OK;
    } else if (atyp == ATYP_DOMAIN) {
        if (data.len < 5) {
            return S5_INCOMPLETE;
        }
        const dlen = data[4] as usize;
        if (dlen == 0) {
            return S5_INVALID;
        }
        const total = 5 + dlen + 2;
        if (data.len < total) {
            return S5_INCOMPLETE;
        }
        const port = ((data[5 + dlen] as u16) << (8 as u4)) | (data[6 + dlen] as u16);
        out.* = Socks5Request{
            cmd: cmd,
            atyp: atyp,
            ipv4: v4,
            ipv6: v6,
            domain: data[5..(5 + dlen)],
            port: port,
            consumed: total,
        };
        return S5_OK;
    } else if (atyp == ATYP_IPV6) {
        if (data.len < 22) {
            return S5_INCOMPLETE;
        }
        let k: usize = 0;
        while (k < 16) {
            v6[k] = data[4 + k];
            k += 1;
        }
        const port = ((data[20] as u16) << (8 as u4)) | (data[21] as u16);
        out.* = Socks5Request{
            cmd: cmd,
            atyp: atyp,
            ipv4: v4,
            ipv6: v6,
            domain: "",
            port: port,
            consumed: 22,
        };
        return S5_OK;
    }

    return S5_INVALID;
}

/// Encode a 10-byte RFC 1928 SOCKS5 IPv4 reply frame `[0x05, rep, 0x00, 0x01, ip[4], port_be]`.
/// Returns `10` on success, or `0` if `out.len < 10`.
pub fn encodeReplyIpv4(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize {
    if (out.len < 10) {
        return 0;
    }
    out[0] = SOCKS_V5;
    out[1] = rep;
    out[2] = 0x00 as u8;
    out[3] = ATYP_IPV4;
    out[4] = bind_ip[0];
    out[5] = bind_ip[1];
    out[6] = bind_ip[2];
    out[7] = bind_ip[3];
    out[8] = ((bind_port >> (8 as u4)) & (0xFF as u16)) as u8;
    out[9] = (bind_port & (0xFF as u16)) as u8;
    return 10;
}

export {
    SOCKS_V5,
    AUTH_NONE,
    AUTH_USERPASS,
    AUTH_NO_ACCEPTABLE,
    CMD_CONNECT,
    CMD_BIND,
    CMD_UDP_ASSOCIATE,
    ATYP_IPV4,
    ATYP_DOMAIN,
    ATYP_IPV6,
    S5_OK,
    S5_INCOMPLETE,
    S5_INVALID,
    Socks5Request,
    encodeGreeting,
    selectMethod,
    encodeUserPassAuth,
    encodeConnectDomain,
    encodeConnectIpv4,
    decodeRequest,
    encodeReplyIpv4,
};
