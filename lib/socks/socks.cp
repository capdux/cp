// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 1928 / RFC 1929 / SOCKS4/4a Wire Protocol Codec)

// Standard SOCKS Port & Protocol Versions
pub const SOCKS_TCP_PORT: u16 = 1080;
pub const SOCKS_V4: u8 = 4;
pub const SOCKS_V5: u8 = 5;
pub const SOCKS5_USERPASS_VER: u8 = 1;

// RFC 1928 Authentication Methods
pub const AUTH_NONE: u8 = 0x00;
pub const AUTH_GSSAPI: u8 = 0x01;
pub const AUTH_USERPASS: u8 = 0x02;
pub const AUTH_NO_ACCEPTABLE: u8 = 0xFF;

// RFC 1928 / SOCKS4 Commands
pub const CMD_CONNECT: u8 = 0x01;
pub const CMD_BIND: u8 = 0x02;
pub const CMD_UDP_ASSOCIATE: u8 = 0x03;

// RFC 1928 Address Types
pub const ATYP_IPV4: u8 = 0x01;
pub const ATYP_DOMAIN: u8 = 0x03;
pub const ATYP_IPV6: u8 = 0x04;

// RFC 1928 Reply Codes
pub const REP_SUCCEEDED: u8 = 0x00;
pub const REP_GENERAL_FAILURE: u8 = 0x01;
pub const REP_NOT_ALLOWED: u8 = 0x02;
pub const REP_NET_UNREACHABLE: u8 = 0x03;
pub const REP_HOST_UNREACHABLE: u8 = 0x04;
pub const REP_CONN_REFUSED: u8 = 0x05;
pub const REP_TTL_EXPIRED: u8 = 0x06;
pub const REP_CMD_NOT_SUPPORTED: u8 = 0x07;
pub const REP_ATYP_NOT_SUPPORTED: u8 = 0x08;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const hi = (buf[off] as u16) << 8;
    const lo = buf[off + 1] as u16;
    return hi | lo;
}

fn writeU16Be(out: []u8, pos: usize, val: u16): usize? {
    if (pos + 2 > out.len) {
        return null;
    }
    out[pos] = ((val >> 8) & 0xFF) as u8;
    out[pos + 1] = (val & 0xFF) as u8;
    return pos + 2;
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn isValidSocks5Cmd(cmd: u8): bool {
    return cmd == CMD_CONNECT || cmd == CMD_BIND || cmd == CMD_UDP_ASSOCIATE;
}

/// RFC 1928 §3 client method-selection greeting (`[VER=5][NMETHODS][METHODS...]`).
pub struct Socks5Greeting {
    methods: []const u8,

    fn supports(method: u8): bool {
        let i: usize = 0;
        while (i < _.methods.len) {
            if (_.methods[i] == method) {
                return true;
            }
            i += 1;
        }
        return false;
    }
}

/// Parses an RFC 1928 SOCKS5 client greeting.
pub fn parseSocks5Greeting(data: []const u8): Socks5Greeting? {
    if (data.len < 2 || data[0] != SOCKS_V5) {
        return null;
    }
    const nmethods = data[1] as usize;
    if (nmethods == 0 || data.len < 2 + nmethods) {
        return null;
    }
    return Socks5Greeting{
        .methods = data[2..(2 + nmethods)],
    };
}

/// Zero-alloc encoder for an RFC 1928 SOCKS5 client greeting.
pub fn encodeSocks5Greeting(methods: []const u8, out: []u8): usize? {
    if (methods.len == 0 || methods.len > 255) {
        return null;
    }
    if (out.len < 2 + methods.len) {
        return null;
    }
    out[0] = SOCKS_V5;
    out[1] = methods.len as u8;
    return writeBytes(out, 2, methods);
}

/// Zero-alloc encoder for an RFC 1928 SOCKS5 server method selection (`[0x05][method]`).
pub fn encodeSocks5MethodSelect(method: u8, out: []u8): usize? {
    if (out.len < 2) {
        return null;
    }
    out[0] = SOCKS_V5;
    out[1] = method;
    return 2;
}

/// RFC 1929 username/password subnegotiation request (`[VER=1][ULEN][UNAME][PLEN][PASSWD]`).
pub struct Socks5UserPass {
    username: []const u8,
    password: []const u8,
}

/// Parses an RFC 1929 username/password authentication message.
pub fn parseSocks5UserPass(data: []const u8): Socks5UserPass? {
    if (data.len < 3 || data[0] != SOCKS5_USERPASS_VER) {
        return null;
    }
    const ulen = data[1] as usize;
    if (ulen == 0 || data.len < 2 + ulen + 1) {
        return null;
    }
    const plen_off = 2 + ulen;
    const plen = data[plen_off] as usize;
    if (plen == 0 || data.len < plen_off + 1 + plen) {
        return null;
    }
    return Socks5UserPass{
        .username = data[2..plen_off],
        .password = data[(plen_off + 1)..(plen_off + 1 + plen)],
    };
}

/// Zero-alloc encoder for an RFC 1929 username/password authentication message.
pub fn encodeSocks5UserPass(user: []const u8, pass: []const u8, out: []u8): usize? {
    if (user.len == 0 || user.len > 255 || pass.len == 0 || pass.len > 255) {
        return null;
    }
    const total = 3 + user.len + pass.len;
    if (out.len < total) {
        return null;
    }
    out[0] = SOCKS5_USERPASS_VER;
    out[1] = user.len as u8;
    let pos = writeBytes(out, 2, user) ?? return null;
    out[pos] = pass.len as u8;
    pos += 1;
    pos = writeBytes(out, pos, pass) ?? return null;
    return pos;
}

/// RFC 1928 §5 address tuple (`ATYP`, address bytes, big-endian `port`).
/// - `ATYP_IPV4`: `raw.len == 4`
/// - `ATYP_DOMAIN`: `1 <= raw.len <= 255` (length-prefixed on the wire)
/// - `ATYP_IPV6`: `raw.len == 16`
pub struct Socks5Addr {
    atyp: u8,
    raw: []const u8,
    port: u16,
}

struct ParsedAddr {
    addr: Socks5Addr,
    next_offset: usize,
}

fn parseAddrAt(data: []const u8, offset: usize): ParsedAddr? {
    if (offset >= data.len) {
        return null;
    }
    const atyp = data[offset];
    if (atyp == ATYP_IPV4) {
        if (data.len < offset + 1 + 4 + 2) {
            return null;
        }
        const raw = data[(offset + 1)..(offset + 5)];
        const port = readU16Be(data, offset + 5);
        return ParsedAddr{
            .addr = Socks5Addr{ .atyp = atyp, .raw = raw, .port = port },
            .next_offset = offset + 7,
        };
    }
    if (atyp == ATYP_DOMAIN) {
        if (data.len < offset + 2) {
            return null;
        }
        const dlen = data[offset + 1] as usize;
        if (dlen == 0 || data.len < offset + 2 + dlen + 2) {
            return null;
        }
        const raw = data[(offset + 2)..(offset + 2 + dlen)];
        const port = readU16Be(data, offset + 2 + dlen);
        return ParsedAddr{
            .addr = Socks5Addr{ .atyp = atyp, .raw = raw, .port = port },
            .next_offset = offset + 4 + dlen,
        };
    }
    if (atyp == ATYP_IPV6) {
        if (data.len < offset + 1 + 16 + 2) {
            return null;
        }
        const raw = data[(offset + 1)..(offset + 17)];
        const port = readU16Be(data, offset + 17);
        return ParsedAddr{
            .addr = Socks5Addr{ .atyp = atyp, .raw = raw, .port = port },
            .next_offset = offset + 19,
        };
    }
    return null;
}

fn encodeAddrAt(addr: Socks5Addr, out: []u8, pos: usize): usize? {
    if (addr.atyp == ATYP_IPV4) {
        if (addr.raw.len != 4 || pos + 7 > out.len) {
            return null;
        }
        out[pos] = ATYP_IPV4;
        let p = writeBytes(out, pos + 1, addr.raw) ?? return null;
        return writeU16Be(out, p, addr.port);
    }
    if (addr.atyp == ATYP_DOMAIN) {
        if (addr.raw.len == 0 || addr.raw.len > 255) {
            return null;
        }
        if (pos + 4 + addr.raw.len > out.len) {
            return null;
        }
        out[pos] = ATYP_DOMAIN;
        out[pos + 1] = addr.raw.len as u8;
        let p = writeBytes(out, pos + 2, addr.raw) ?? return null;
        return writeU16Be(out, p, addr.port);
    }
    if (addr.atyp == ATYP_IPV6) {
        if (addr.raw.len != 16 || pos + 19 > out.len) {
            return null;
        }
        out[pos] = ATYP_IPV6;
        let p = writeBytes(out, pos + 1, addr.raw) ?? return null;
        return writeU16Be(out, p, addr.port);
    }
    return null;
}

/// RFC 1928 §4 SOCKS5 client request (`[VER=5][CMD][RSV=0][ATYP][DST.ADDR][DST.PORT]`).
pub struct Socks5Request {
    cmd: u8,
    addr: Socks5Addr,
}

/// Parses an RFC 1928 SOCKS5 request. Rejects invalid version, unknown command, non-zero RSV, or invalid address.
pub fn parseSocks5Request(data: []const u8): Socks5Request? {
    if (data.len < 4 || data[0] != SOCKS_V5 || data[2] != 0) {
        return null;
    }
    const cmd = data[1];
    if (!isValidSocks5Cmd(cmd)) {
        return null;
    }
    const parsed = parseAddrAt(data, 3) ?? return null;
    return Socks5Request{
        .cmd = cmd,
        .addr = parsed.addr,
    };
}

/// Zero-alloc encoder for an RFC 1928 SOCKS5 request.
pub fn encodeSocks5Request(cmd: u8, addr: Socks5Addr, out: []u8): usize? {
    if (!isValidSocks5Cmd(cmd) || out.len < 4) {
        return null;
    }
    out[0] = SOCKS_V5;
    out[1] = cmd;
    out[2] = 0;
    return encodeAddrAt(addr, out, 3);
}

/// RFC 1928 §6 SOCKS5 server reply (`[VER=5][REP][RSV=0][ATYP][BND.ADDR][BND.PORT]`).
pub struct Socks5Reply {
    rep: u8,
    bnd_addr: Socks5Addr,
}

/// Parses an RFC 1928 SOCKS5 reply. Rejects invalid version, non-zero RSV, or invalid address.
pub fn parseSocks5Reply(data: []const u8): Socks5Reply? {
    if (data.len < 4 || data[0] != SOCKS_V5 || data[2] != 0) {
        return null;
    }
    const parsed = parseAddrAt(data, 3) ?? return null;
    return Socks5Reply{
        .rep = data[1],
        .bnd_addr = parsed.addr,
    };
}

/// Zero-alloc encoder for an RFC 1928 SOCKS5 reply.
pub fn encodeSocks5Reply(rep: u8, addr: Socks5Addr, out: []u8): usize? {
    if (out.len < 4) {
        return null;
    }
    out[0] = SOCKS_V5;
    out[1] = rep;
    out[2] = 0;
    return encodeAddrAt(addr, out, 3);
}

/// RFC 1928 §7 SOCKS5 UDP relay datagram (`[RSV=0,0][FRAG][ATYP][DST.ADDR][DST.PORT][DATA]`).
pub struct Socks5UdpPacket {
    frag: u8,
    dst_addr: Socks5Addr,
    payload: []const u8,
}

/// Parses an RFC 1928 SOCKS5 UDP relay packet with zero-copy payload slicing.
pub fn parseSocks5Udp(data: []const u8): Socks5UdpPacket? {
    if (data.len < 4 || data[0] != 0 || data[1] != 0) {
        return null;
    }
    const parsed = parseAddrAt(data, 3) ?? return null;
    return Socks5UdpPacket{
        .frag = data[2],
        .dst_addr = parsed.addr,
        .payload = data[parsed.next_offset..],
    };
}

/// Zero-alloc encoder for an RFC 1928 SOCKS5 UDP relay packet.
pub fn encodeSocks5Udp(frag: u8, dst_addr: Socks5Addr, payload: []const u8, out: []u8): usize? {
    if (out.len < 4) {
        return null;
    }
    out[0] = 0;
    out[1] = 0;
    out[2] = frag;
    const pos = encodeAddrAt(dst_addr, out, 3) ?? return null;
    return writeBytes(out, pos, payload);
}

/// SOCKS4 / SOCKS4a client request (`[VN=4][CD][PORT:2B][IP:4B][USERID\0]([DOMAIN\0])`).
pub struct Socks4Request {
    cmd: u8,
    port: u16,
    ip: [4]u8,
    user_id: []const u8,
    domain: []const u8,

    fn isSocks4a(): bool {
        return _.ip[0] == 0 && _.ip[1] == 0 && _.ip[2] == 0 && _.ip[3] != 0;
    }
}

fn findNul(data: []const u8, start: usize): usize? {
    let i = start;
    while (i < data.len) {
        if (data[i] == 0) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Parses a SOCKS4 or SOCKS4a client request.
/// When `ip` matches `0.0.0.x` (`x != 0`), extracts the trailing NUL-terminated SOCKS4a domain.
pub fn parseSocks4Request(data: []const u8): Socks4Request? {
    if (data.len < 9 || data[0] != SOCKS_V4) {
        return null;
    }
    const cmd = data[1];
    if (cmd != CMD_CONNECT && cmd != CMD_BIND) {
        return null;
    }
    const port = readU16Be(data, 2);
    const ip: [4]u8 = [data[4], data[5], data[6], data[7]];
    const user_nul = findNul(data, 8) ?? return null;
    const user_id = data[8..user_nul];

    let domain: []const u8 = data[0..0];
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] != 0) {
        const dom_start = user_nul + 1;
        const dom_nul = findNul(data, dom_start) ?? return null;
        if (dom_nul == dom_start) {
            return null;
        }
        domain = data[dom_start..dom_nul];
    }

    return Socks4Request{
        .cmd = cmd,
        .port = port,
        .ip = ip,
        .user_id = user_id,
        .domain = domain,
    };
}
