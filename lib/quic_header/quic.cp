// Source: Absorbed from /data/dev/bun/src/uws_sys/quic/Header.cp | Synced: 2026-11-21T12:31:02Z

/// QUIC v1 版本号（RFC 9000）
pub const QUIC_VERSION_1: u32 = 0x00000001;
/// QUIC v2 版本号（RFC 9369）
pub const QUIC_VERSION_2: u32 = 0x6b3343cf;
/// QUIC 版本协商报文魔数（0x00000000）
pub const QUIC_VERSION_NEGOTIATION: u32 = 0x00000000;
/// RFC 9000 §17.2 允许的最大连接 ID 长度（20 字节）
pub const QUIC_MAX_CID_LEN: usize = 20;
/// RFC 9000 §16 变长整数最大值（2^62 - 1）
pub const QUIC_VARINT_MAX: u64 = 4611686018427387903;

/// QUIC 报文头与 Socket 类型解析错误集
pub err QuicHeaderError {
    UnexpectedEof,
    InvalidFixedBit,
    InvalidCidLength,
    VarIntTooLarge,
    BufferTooSmall,
    InvalidSocketKind,
}

/// QUIC 传输层报文类型（RFC 9000 §17）
pub enum QuicPacketKind: u8 {
    Initial = 0,
    ZeroRTT = 1,
    Handshake = 2,
    Retry = 3,
    VersionNegotiation = 4,
    Short = 5,

    fn isLongHeader(): bool {
        return _ != .Short;
    }

    fn isShortHeader(): bool {
        return _ == .Short;
    }
}

/// QUIC 变长整数解码结果（RFC 9000 §16）
pub struct DecodedVarInt {
    value: u64,
    bytes_read: usize,
}

/// 计算 QUIC 变长整数编码所需字节数（1 / 2 / 4 / 8 字节）。
pub fn varIntEncodedLen(value: u64): usize ! QuicHeaderError {
    if (value <= 63) {
        return 1;
    }
    if (value <= 16383) {
        return 2;
    }
    if (value <= 1073741823) {
        return 4;
    }
    if (value <= QUIC_VARINT_MAX) {
        return 8;
    }
    return err.VarIntTooLarge;
}

/// 编码 QUIC 变长整数到大端缓冲区（RFC 9000 §16）。
pub fn encodeVarInt(buf: []u8, value: u64): usize ! QuicHeaderError {
    const need = varIntEncodedLen(value)?;
    if (buf.len < need) {
        return err.BufferTooSmall;
    }
    if (need == 1) {
        buf[0] = (value & 0x3F) as u8;
        return 1;
    }
    if (need == 2) {
        buf[0] = u8(0x40) | (((value >> 8) & 0x3F) as u8);
        buf[1] = (value & 0xFF) as u8;
        return 2;
    }
    if (need == 4) {
        buf[0] = u8(0x80) | (((value >> 24) & 0x3F) as u8);
        buf[1] = ((value >> 16) & 0xFF) as u8;
        buf[2] = ((value >> 8) & 0xFF) as u8;
        buf[3] = (value & 0xFF) as u8;
        return 4;
    }
    buf[0] = u8(0xC0) | (((value >> 56) & 0x3F) as u8);
    buf[1] = ((value >> 48) & 0xFF) as u8;
    buf[2] = ((value >> 40) & 0xFF) as u8;
    buf[3] = ((value >> 32) & 0xFF) as u8;
    buf[4] = ((value >> 24) & 0xFF) as u8;
    buf[5] = ((value >> 16) & 0xFF) as u8;
    buf[6] = ((value >> 8) & 0xFF) as u8;
    buf[7] = (value & 0xFF) as u8;
    return 8;
}

/// 从大端字节流解码 QUIC 变长整数（RFC 9000 §16）。
pub fn decodeVarInt(data: []const u8): DecodedVarInt ! QuicHeaderError {
    if (data.len == 0) {
        return err.UnexpectedEof;
    }
    const prefix = data[0] >> 6;
    const len: usize = switch (prefix) {
        0 => 1,
        1 => 2,
        2 => 4,
        else => 8,
    };
    if (data.len < len) {
        return err.UnexpectedEof;
    }
    let val: u64 = (data[0] & 0x3F) as u64;
    let i: usize = 1;
    while (i < len) {
        val = (val << 8) | (data[i] as u64);
        i += 1;
    }
    return DecodedVarInt{
        value: val,
        bytes_read: len,
    };
}

/// QUIC 传输层报文头结构（兼容长头 Initial/0-RTT/Handshake/Retry/VersionNegotiation 与 1-RTT 短头）
pub struct QuicHeader {
    kind: QuicPacketKind,
    is_long: bool,
    fixed_bit: bool = true,
    spin_bit: bool = false,
    key_phase: bool = false,
    packet_number_len: u8 = 1,
    version: u32 = QUIC_VERSION_1,
    dcid: []const u8 = [],
    scid: []const u8 = [],
    token: []const u8 = [],
    header_len: usize = 0,

    fn isLongHeader(): bool {
        return _.is_long;
    }

    fn isShortHeader(): bool {
        return !_.is_long;
    }

    fn isSupportedVersion(): bool {
        return _.version == QUIC_VERSION_1 || _.version == QUIC_VERSION_2;
    }
}

fn readU32BE(data: []const u8, offset: usize): u32 {
    return ((data[offset] as u32) << 24)
        | ((data[offset + 1] as u32) << 16)
        | ((data[offset + 2] as u32) << 8)
        | (data[offset + 3] as u32);
}

fn writeU32BE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = ((val >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 3] = (val & 0xFF) as u8;
}

/// 从字节流解码 QUIC 长头或短头（`short_dcid_len` 为短头上下文已知的 DCID 长度，0..20）。
pub fn decodeQuicHeader(data: []const u8, short_dcid_len: usize): QuicHeader ! QuicHeaderError {
    if (data.len < 1) {
        return err.UnexpectedEof;
    }
    const first = data[0];
    const is_long = (first & 0x80) != 0;
    const fixed_bit = (first & 0x40) != 0;

    if (is_long) {
        // 至少需要：1B 首字节 + 4B 版本 + 1B DCID 长度
        if (data.len < 6) {
            return err.UnexpectedEof;
        }
        const version = readU32BE(data, 1);
        if (version != QUIC_VERSION_NEGOTIATION && !fixed_bit) {
            return err.InvalidFixedBit;
        }

        let pos: usize = 5;
        const dcid_len: usize = data[pos] as usize;
        pos += 1;
        if (dcid_len > QUIC_MAX_CID_LEN) {
            return err.InvalidCidLength;
        }
        if (pos + dcid_len + 1 > data.len) {
            return err.UnexpectedEof;
        }
        const dcid = data[pos .. pos + dcid_len];
        pos += dcid_len;

        const scid_len: usize = data[pos] as usize;
        pos += 1;
        if (scid_len > QUIC_MAX_CID_LEN) {
            return err.InvalidCidLength;
        }
        if (pos + scid_len > data.len) {
            return err.UnexpectedEof;
        }
        const scid = data[pos .. pos + scid_len];
        pos += scid_len;

        if (version == QUIC_VERSION_NEGOTIATION) {
            return QuicHeader{
                kind: QuicPacketKind.VersionNegotiation,
                is_long: true,
                fixed_bit: fixed_bit,
                packet_number_len: 0,
                version: version,
                dcid: dcid,
                scid: scid,
                token: [],
                header_len: pos,
            };
        }

        const raw_type = (first >> 4) & 0x03;
        const kind: QuicPacketKind = switch (raw_type) {
            0 => QuicPacketKind.Initial,
            1 => QuicPacketKind.ZeroRTT,
            2 => QuicPacketKind.Handshake,
            else => QuicPacketKind.Retry,
        };

        let token: []const u8 = [];
        if (kind == QuicPacketKind.Initial) {
            const tok_vi = decodeVarInt(data[pos..])?;
            pos += tok_vi.bytes_read;
            const tok_len: usize = tok_vi.value as usize;
            if (pos + tok_len > data.len) {
                return err.UnexpectedEof;
            }
            token = data[pos .. pos + tok_len];
            pos += tok_len;
        }

        const pn_len: u8 = if (kind == QuicPacketKind.Retry) 0 else ((first & 0x03) + 1);
        return QuicHeader{
            kind: kind,
            is_long: true,
            fixed_bit: fixed_bit,
            packet_number_len: pn_len,
            version: version,
            dcid: dcid,
            scid: scid,
            token: token,
            header_len: pos,
        };
    }

    // 1-RTT Short Header
    if (!fixed_bit) {
        return err.InvalidFixedBit;
    }
    if (short_dcid_len > QUIC_MAX_CID_LEN) {
        return err.InvalidCidLength;
    }
    if (1 + short_dcid_len > data.len) {
        return err.UnexpectedEof;
    }
    const spin_bit = (first & 0x20) != 0;
    const key_phase = (first & 0x04) != 0;
    const pn_len: u8 = (first & 0x03) + 1;
    return QuicHeader{
        kind: QuicPacketKind.Short,
        is_long: false,
        fixed_bit: true,
        spin_bit: spin_bit,
        key_phase: key_phase,
        packet_number_len: pn_len,
        version: 0,
        dcid: data[1 .. 1 + short_dcid_len],
        scid: [],
        token: [],
        header_len: 1 + short_dcid_len,
    };
}

/// 将 `QuicHeader` 编码为 RFC 9000 大端字节流，返回写入字节数。
pub fn encodeQuicHeader(buf: []u8, hdr: QuicHeader): usize ! QuicHeaderError {
    if (hdr.dcid.len > QUIC_MAX_CID_LEN || hdr.scid.len > QUIC_MAX_CID_LEN) {
        return err.InvalidCidLength;
    }

    if (!hdr.is_long || hdr.kind == QuicPacketKind.Short) {
        const need: usize = usize(1) + hdr.dcid.len;
        if (buf.len < need) {
            return err.BufferTooSmall;
        }
        const pn_bits: u8 = if (hdr.packet_number_len >= 1 && hdr.packet_number_len <= 4) (hdr.packet_number_len - 1) else 0;
        let first: u8 = u8(0x40) | pn_bits;
        if (hdr.spin_bit) {
            first |= 0x20;
        }
        if (hdr.key_phase) {
            first |= 0x04;
        }
        buf[0] = first;
        if (hdr.dcid.len > 0) {
            @memcpy(buf[1 .. usize(1) + hdr.dcid.len], hdr.dcid);
        }
        return need;
    }

    // Long Header
    const base_need: usize = usize(7) + hdr.dcid.len + hdr.scid.len;
    let tok_len_bytes: usize = 0;
    let tok_bytes: usize = 0;
    if (hdr.kind == QuicPacketKind.Initial) {
        tok_len_bytes = varIntEncodedLen(hdr.token.len as u64)?;
        tok_bytes = hdr.token.len;
    }
    const total_need = base_need + tok_len_bytes + tok_bytes;
    if (buf.len < total_need) {
        return err.BufferTooSmall;
    }

    const type_bits: u8 = switch (hdr.kind) {
        .Initial => 0,
        .ZeroRTT => 1,
        .Handshake => 2,
        .Retry => 3,
        else => 0,
    };
    const pn_bits: u8 = if (hdr.packet_number_len >= 1 && hdr.packet_number_len <= 4) (hdr.packet_number_len - 1) else 0;
    let first: u8 = u8(0x80) | (type_bits << 4) | pn_bits;
    if (hdr.fixed_bit || hdr.kind != QuicPacketKind.VersionNegotiation) {
        first |= 0x40;
    }
    buf[0] = first;

    const ver: u32 = if (hdr.kind == QuicPacketKind.VersionNegotiation) QUIC_VERSION_NEGOTIATION else hdr.version;
    writeU32BE(buf, 1, ver);

    let pos: usize = 5;
    buf[pos] = hdr.dcid.len as u8;
    pos += 1;
    if (hdr.dcid.len > 0) {
        @memcpy(buf[pos .. pos + hdr.dcid.len], hdr.dcid);
        pos += hdr.dcid.len;
    }
    buf[pos] = hdr.scid.len as u8;
    pos += 1;
    if (hdr.scid.len > 0) {
        @memcpy(buf[pos .. pos + hdr.scid.len], hdr.scid);
        pos += hdr.scid.len;
    }

    if (hdr.kind == QuicPacketKind.Initial) {
        const vi_written = encodeVarInt(buf[pos..], hdr.token.len as u64)?;
        pos += vi_written;
        if (hdr.token.len > 0) {
            @memcpy(buf[pos .. pos + hdr.token.len], hdr.token);
            pos += hdr.token.len;
        }
    }
    return pos;
}

/// HTTP/3 QPACK 静态表头分类结果（吸收自 Bun `uws_sys/quic/Header.cp`）
pub enum QpackHeaderClass: u8 {
    Normal = 0,
    Forbidden = 1,
    Host = 2,
    Indexed = 3,
}

/// HTTP/3 QPACK 静态表索引（RFC 9204 常用静态头）
pub enum QpackIndex: u8 {
    authority = 0,
    path = 1,
    content_disposition = 3,
    content_length = 4,
    cookie = 5,
    date = 6,
    etag = 7,
    if_modified_since = 8,
    if_none_match = 9,
    last_modified = 10,
    link = 11,
    location = 12,
    referer = 13,
    set_cookie = 14,
    method_get = 17,
    scheme_https = 23,
    accept = 29,
    accept_encoding = 31,
    accept_ranges = 32,
    cache_control = 36,
    content_encoding = 43,
    content_type = 44,
    range = 55,
    vary = 59,
    accept_language = 72,
    authorization = 84,
    forwarded = 88,
    if_range = 89,
    origin = 90,
    server = 92,
    user_agent = 95,
    x_forwarded_for = 96,
}

/// QPACK 请求头分类结果
pub struct QpackLookup {
    class: QpackHeaderClass,
    index: QpackIndex?,
}

/// 忽略 ASCII 大小写判断 HTTP/3 请求头的 QPACK 静态表归类（禁止头 / Host / 静态索引头 / 普通头）。
pub fn classifyQpackHeader(name: string): QpackLookup {
    if (std.ascii.eqlIgnoreCase(name, "connection")
        || std.ascii.eqlIgnoreCase(name, "keep-alive")
        || std.ascii.eqlIgnoreCase(name, "proxy-connection")
        || std.ascii.eqlIgnoreCase(name, "transfer-encoding")
        || std.ascii.eqlIgnoreCase(name, "upgrade"))
    {
        return QpackLookup{ class: QpackHeaderClass.Forbidden, index: null };
    }
    if (std.ascii.eqlIgnoreCase(name, "host")) {
        return QpackLookup{ class: QpackHeaderClass.Host, index: null };
    }
    if (std.ascii.eqlIgnoreCase(name, "accept")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.accept };
    }
    if (std.ascii.eqlIgnoreCase(name, "accept-encoding")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.accept_encoding };
    }
    if (std.ascii.eqlIgnoreCase(name, "accept-language")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.accept_language };
    }
    if (std.ascii.eqlIgnoreCase(name, "authorization")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.authorization };
    }
    if (std.ascii.eqlIgnoreCase(name, "cache-control")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.cache_control };
    }
    if (std.ascii.eqlIgnoreCase(name, "content-length")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.content_length };
    }
    if (std.ascii.eqlIgnoreCase(name, "content-type")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.content_type };
    }
    if (std.ascii.eqlIgnoreCase(name, "cookie")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.cookie };
    }
    if (std.ascii.eqlIgnoreCase(name, "etag")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.etag };
    }
    if (std.ascii.eqlIgnoreCase(name, "origin")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.origin };
    }
    if (std.ascii.eqlIgnoreCase(name, "server")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.server };
    }
    if (std.ascii.eqlIgnoreCase(name, "user-agent")) {
        return QpackLookup{ class: QpackHeaderClass.Indexed, index: QpackIndex.user_agent };
    }
    return QpackLookup{ class: QpackHeaderClass.Normal, index: null };
}

/// Socket 类型枚举（吸收自 Bun `uws_sys/SocketKind.cp`）
pub enum SocketKind: u8 {
    invalid = 0,
    dynamic = 1,
    socket_tcp = 2,
    socket_tls = 3,
    listener_tcp = 4,
    listener_tls = 5,
    http_client = 6,
    http_client_tls = 7,
    ws_client_upgrade = 8,
    ws_client_upgrade_tls = 9,
    ws_client = 10,
    ws_client_tls = 11,
    postgres = 12,
    postgres_tls = 13,
    mysql = 14,
    mysql_tls = 15,
    valkey = 16,
    valkey_tls = 17,
    spawn_ipc = 18,
    uws_http = 19,
    uws_http_tls = 20,
    uws_ws = 21,
    uws_ws_tls = 22,

    fn isValid(): bool {
        return _ != .invalid;
    }

    fn isTLS(): bool {
        return switch (_) {
            .socket_tls,
            .listener_tls,
            .http_client_tls,
            .ws_client_upgrade_tls,
            .ws_client_tls,
            .postgres_tls,
            .mysql_tls,
            .valkey_tls,
            .uws_http_tls,
            .uws_ws_tls => true,
            else => false,
        };
    }

    fn isListener(): bool {
        return _ == .listener_tcp || _ == .listener_tls;
    }

    fn isWebSocket(): bool {
        return switch (_) {
            .ws_client_upgrade,
            .ws_client_upgrade_tls,
            .ws_client,
            .ws_client_tls,
            .uws_ws,
            .uws_ws_tls => true,
            else => false,
        };
    }

    fn isHttp(): bool {
        return switch (_) {
            .http_client,
            .http_client_tls,
            .uws_http,
            .uws_http_tls => true,
            else => false,
        };
    }

    fn isDatabase(): bool {
        return switch (_) {
            .postgres,
            .postgres_tls,
            .mysql,
            .mysql_tls,
            .valkey,
            .valkey_tls => true,
            else => false,
        };
    }

    fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

/// 从字节解析 `SocketKind` 枚举，越界值返回 `InvalidSocketKind`。
pub fn socketKindFromByte(b: u8): SocketKind ! QuicHeaderError {
    if (b > 22) {
        return err.InvalidSocketKind;
    }
    return @enumFromInt(b);
}
