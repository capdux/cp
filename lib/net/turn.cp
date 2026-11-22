// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8656 TURN Wire Protocol, ChannelData & XOR-ADDRESS Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (RFC 8656 / RFC 5389)
// ---------------------------------------------------------------------------

pub const TURN_UDP_PORT: u16 = 3478;
pub const TURNS_TLS_PORT: u16 = 5349;
pub const TURN_MAGIC_COOKIE: u32 = 0x2112A442;
pub const TURN_HEADER_SIZE: usize = 20;
pub const CHANNEL_DATA_HEADER_SIZE: usize = 4;
pub const CHANNEL_MIN: u16 = 0x4000;
pub const CHANNEL_MAX: u16 = 0x7FFF;

// TURN Methods (12-bit)
pub const METHOD_ALLOCATE: u16 = 0x003;
pub const METHOD_REFRESH: u16 = 0x004;
pub const METHOD_SEND: u16 = 0x006;
pub const METHOD_DATA: u16 = 0x007;
pub const METHOD_CREATE_PERMISSION: u16 = 0x008;
pub const METHOD_CHANNEL_BIND: u16 = 0x009;

// STUN/TURN Message Classes (2-bit)
pub const CLASS_REQUEST: u8 = 0;
pub const CLASS_INDICATION: u8 = 1;
pub const CLASS_SUCCESS: u8 = 2;
pub const CLASS_ERROR: u8 = 3;

// TURN Attribute Types
pub const ATTR_ERROR_CODE: u16 = 0x0009;
pub const ATTR_CHANNEL_NUMBER: u16 = 0x000C;
pub const ATTR_LIFETIME: u16 = 0x000D;
pub const ATTR_XOR_PEER_ADDRESS: u16 = 0x0012;
pub const ATTR_DATA: u16 = 0x0013;
pub const ATTR_XOR_RELAYED_ADDRESS: u16 = 0x0016;
pub const ATTR_REQUESTED_ADDR_FAMILY: u16 = 0x0017;
pub const ATTR_EVEN_PORT: u16 = 0x0018;
pub const ATTR_REQUESTED_TRANSPORT: u16 = 0x0019;
pub const ATTR_DONT_FRAGMENT: u16 = 0x001A;
pub const ATTR_RESERVATION_TOKEN: u16 = 0x0022;

// Requested Transport Protocol Numbers
pub const TRANSPORT_TCP: u8 = 6;
pub const TRANSPORT_UDP: u8 = 17;

// Address Families for XOR-ADDRESS
pub const ADDR_FAMILY_IPV4: u8 = 0x01;
pub const ADDR_FAMILY_IPV6: u8 = 0x02;

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU16Be(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn padded4(n: usize): usize {
    const rem = n % 4;
    if (rem == 0) {
        return n;
    }
    return n + (4 - rem);
}

/// Encodes a 12-bit STUN/TURN method and 2-bit message class into the 16-bit wire `msg_type`.
pub fn encodeTurnMsgType(method: u16, msg_class: u8): u16 {
    const m = method & 0x0FFF;
    const c = (msg_class as u16) & 0x03;
    const m_lo = m & 0x000F;
    const m_mid = (m & 0x0070) << 1;
    const m_hi = (m & 0x0F80) << 2;
    const c0 = (c & 0x01) << 4;
    const c1 = (c & 0x02) << 7;
    return m_lo | m_mid | m_hi | c0 | c1;
}

// ---------------------------------------------------------------------------
// TURN Header, Attributes & Message
// ---------------------------------------------------------------------------

/// Decoded 20-byte STUN/TURN message header.
pub struct TurnHeader {
    msg_type: u16,
    method: u16,
    msg_class: u8,
    length: u16,
    tx_id: [12]u8,

    fn isRequest(): bool {
        return _.msg_class == CLASS_REQUEST;
    }

    fn isIndication(): bool {
        return _.msg_class == CLASS_INDICATION;
    }

    fn isSuccess(): bool {
        return _.msg_class == CLASS_SUCCESS;
    }

    fn isError(): bool {
        return _.msg_class == CLASS_ERROR;
    }
}

/// A single zero-copy TURN TLV attribute.
pub struct TurnAttr {
    attr_type: u16,
    length: u16,
    value: []const u8,
}

/// Zero-allocation iterator over 4-byte-padded TURN attributes.
pub struct TurnAttrIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    fn init(attrs_bytes: []const u8): TurnAttrIter {
        return TurnAttrIter {
            data: attrs_bytes,
            offset: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): TurnAttr? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        if (_.offset + 4 > _.data.len) {
            _.failed = true;
            return null;
        }
        const attr_type = readU16Be(_.data, _.offset);
        const length = readU16Be(_.data, _.offset + 2);
        const val_start = _.offset + 4;
        const val_len = length as usize;
        const pad_len = padded4(val_len);
        if (val_start + pad_len > _.data.len) {
            _.failed = true;
            return null;
        }
        const value = _.data[val_start..val_start + val_len];
        _.offset = val_start + pad_len;
        return TurnAttr {
            attr_type: attr_type,
            length: length,
            value: value,
        };
    }
}

// ---------------------------------------------------------------------------
// XOR-ADDRESS Codec (RFC 5389 §15.2 / RFC 8656)
// ---------------------------------------------------------------------------

/// Decoded IPv4 or IPv6 XOR-PEER-ADDRESS / XOR-RELAYED-ADDRESS.
/// For IPv4, bytes are stored in `addr[0..4]` and `addr[4..16]` are zeroed.
pub struct TurnXorAddr {
    is_ipv6: bool,
    port: u16,
    addr: [16]u8,

    fn ipv4Bytes(): [4]u8 {
        return [_.addr[0], _.addr[1], _.addr[2], _.addr[3]];
    }
}

/// Decodes a TURN/STUN XOR-ADDRESS attribute value using the magic cookie and 12-byte `tx_id`.
pub fn decodeXorAddr(value: []const u8, tx_id: [12]u8): TurnXorAddr? {
    if (value.len < 8) {
        return null;
    }
    const family = value[1];
    const x_port = readU16Be(value, 2);
    const cookie_hi: u16 = ((TURN_MAGIC_COOKIE >> 16) & 0xFFFF) as u16;
    const port = x_port ^ cookie_hi;

    const cookie_bytes: [4]u8 = [0x21, 0x12, 0xA4, 0x42];
    let addr: [16]u8 = [0; 16];

    if (family == ADDR_FAMILY_IPV4) {
        if (value.len != 8) {
            return null;
        }
        let i: usize = 0;
        while (i < 4) {
            addr[i] = value[4 + i] ^ cookie_bytes[i];
            i += 1;
        }
        return TurnXorAddr {
            is_ipv6: false,
            port: port,
            addr: addr,
        };
    } else if (family == ADDR_FAMILY_IPV6) {
        if (value.len != 20) {
            return null;
        }
        let i: usize = 0;
        while (i < 4) {
            addr[i] = value[4 + i] ^ cookie_bytes[i];
            i += 1;
        }
        let j: usize = 0;
        while (j < 12) {
            addr[4 + j] = value[8 + j] ^ tx_id[j];
            j += 1;
        }
        return TurnXorAddr {
            is_ipv6: true,
            port: port,
            addr: addr,
        };
    }

    return null;
}

/// Encodes a `TurnXorAddr` into `out` (8 bytes for IPv4, 20 bytes for IPv6).
/// Returns the number of bytes written, or `null` if `out` is too small.
pub fn encodeXorAddr(addr: TurnXorAddr, tx_id: [12]u8, out: []u8): usize? {
    const need: usize = if (addr.is_ipv6) 20 else 8;
    if (out.len < need) {
        return null;
    }
    const cookie_hi: u16 = ((TURN_MAGIC_COOKIE >> 16) & 0xFFFF) as u16;
    const cookie_bytes: [4]u8 = [0x21, 0x12, 0xA4, 0x42];

    out[0] = 0;
    out[1] = if (addr.is_ipv6) ADDR_FAMILY_IPV6 else ADDR_FAMILY_IPV4;
    writeU16Be(out, 2, addr.port ^ cookie_hi);

    let i: usize = 0;
    while (i < 4) {
        out[4 + i] = addr.addr[i] ^ cookie_bytes[i];
        i += 1;
    }
    if (addr.is_ipv6) {
        let j: usize = 0;
        while (j < 12) {
            out[8 + j] = addr.addr[4 + j] ^ tx_id[j];
            j += 1;
        }
    }
    return need;
}

// ---------------------------------------------------------------------------
// TURN Message View & Parser
// ---------------------------------------------------------------------------

/// Zero-copy view of a parsed STUN/TURN message.
pub struct TurnMessage {
    header: TurnHeader,
    attrs_bytes: []const u8,

    fn attrs(): TurnAttrIter {
        return TurnAttrIter.init(_.attrs_bytes);
    }

    fn findAttr(attr_type: u16): TurnAttr? {
        let it = _.attrs();
        while (attr := it.next()) {
            if (attr.attr_type == attr_type) {
                return attr;
            }
        }
        return null;
    }

    /// Returns the 32-bit LIFETIME attribute value in seconds, if present and valid (4 bytes).
    fn lifetime(): u32? {
        if (attr := _.findAttr(ATTR_LIFETIME)) {
            if (attr.value.len == 4) {
                return readU32Be(attr.value, 0);
            }
        }
        return null;
    }

    /// Returns the 8-bit protocol number from REQUESTED-TRANSPORT (e.g., 17=UDP, 6=TCP).
    fn requestedTransport(): u8? {
        if (attr := _.findAttr(ATTR_REQUESTED_TRANSPORT)) {
            if (attr.value.len == 4) {
                return attr.value[0];
            }
        }
        return null;
    }

    /// Returns the 16-bit channel number from CHANNEL-NUMBER if in `0x4000..0x7FFF`.
    fn channelNumber(): u16? {
        if (attr := _.findAttr(ATTR_CHANNEL_NUMBER)) {
            if (attr.value.len == 4) {
                const ch = readU16Be(attr.value, 0);
                if (ch >= CHANNEL_MIN && ch <= CHANNEL_MAX) {
                    return ch;
                }
            }
        }
        return null;
    }

    /// Returns the raw payload bytes of the DATA attribute, if present.
    fn dataAttr(): ([]const u8)? {
        if (attr := _.findAttr(ATTR_DATA)) {
            return attr.value;
        }
        return null;
    }

    /// Decodes the XOR-RELAYED-ADDRESS attribute using this message's transaction ID.
    fn xorRelayedAddress(): TurnXorAddr? {
        if (attr := _.findAttr(ATTR_XOR_RELAYED_ADDRESS)) {
            return decodeXorAddr(attr.value, _.header.tx_id);
        }
        return null;
    }

    /// Decodes the XOR-PEER-ADDRESS attribute using this message's transaction ID.
    fn xorPeerAddress(): TurnXorAddr? {
        if (attr := _.findAttr(ATTR_XOR_PEER_ADDRESS)) {
            return decodeXorAddr(attr.value, _.header.tx_id);
        }
        return null;
    }
}

/// Fast-path validation for a STUN/TURN wire message.
pub fn isTurnMessage(data: []const u8): bool {
    if (data.len < TURN_HEADER_SIZE) {
        return false;
    }
    const msg_type = readU16Be(data, 0);
    if ((msg_type & 0xC000) != 0) {
        return false;
    }
    const length = readU16Be(data, 2);
    if ((length & 0x0003) != 0) {
        return false;
    }
    const cookie = readU32Be(data, 4);
    if (cookie != TURN_MAGIC_COOKIE) {
        return false;
    }
    if (TURN_HEADER_SIZE + (length as usize) > data.len) {
        return false;
    }
    return true;
}

/// Parses a STUN/TURN wire message into a zero-copy `TurnMessage`.
/// Strictly validates top 2 zero bits, magic cookie (`0x2112A442`), 4-byte aligned length,
/// and `20 + length <= data.len`.
pub fn parseTurnMessage(data: []const u8): TurnMessage? {
    if (!isTurnMessage(data)) {
        return null;
    }
    const msg_type = readU16Be(data, 0);
    const length = readU16Be(data, 2);

    const c0 = (msg_type >> 4) & 0x01;
    const c1 = (msg_type >> 8) & 0x01;
    const msg_class = ((c1 << 1) | c0) as u8;
    const method = (msg_type & 0x000F) | ((msg_type & 0x00E0) >> 1) | ((msg_type & 0x3E00) >> 2);

    let tx_id: [12]u8 = [0; 12];
    let i: usize = 0;
    while (i < 12) {
        tx_id[i] = data[8 + i];
        i += 1;
    }

    const attrs_end = TURN_HEADER_SIZE + (length as usize);
    return TurnMessage {
        header: TurnHeader {
            msg_type: msg_type,
            method: method,
            msg_class: msg_class,
            length: length,
            tx_id: tx_id,
        },
        attrs_bytes: data[TURN_HEADER_SIZE..attrs_end],
    };
}

// ---------------------------------------------------------------------------
// ChannelData Framing (RFC 8656 §12.4)
// ---------------------------------------------------------------------------

/// Zero-copy view of a TURN ChannelData frame.
pub struct TurnChannelData {
    channel: u16,
    length: u16,
    payload: []const u8,
}

/// Validates whether `data` starts with a well-formed TURN ChannelData frame.
pub fn isChannelData(data: []const u8): bool {
    if (data.len < CHANNEL_DATA_HEADER_SIZE) {
        return false;
    }
    const ch = readU16Be(data, 0);
    if (ch < CHANNEL_MIN || ch > CHANNEL_MAX) {
        return false;
    }
    const length = readU16Be(data, 2);
    if (CHANNEL_DATA_HEADER_SIZE + (length as usize) > data.len) {
        return false;
    }
    return true;
}

/// Parses a TURN ChannelData frame (`0x4000 <= channel <= 0x7FFF` and `4 + length <= data.len`).
pub fn parseChannelData(data: []const u8): TurnChannelData? {
    if (!isChannelData(data)) {
        return null;
    }
    const ch = readU16Be(data, 0);
    const length = readU16Be(data, 2);
    const end = CHANNEL_DATA_HEADER_SIZE + (length as usize);
    return TurnChannelData {
        channel: ch,
        length: length,
        payload: data[CHANNEL_DATA_HEADER_SIZE..end],
    };
}

/// Encodes a TURN ChannelData frame into `out`.
/// Validates `0x4000 <= channel <= 0x7FFF`, `payload.len <= 65535`, and `out.len >= 4 + payload.len`.
pub fn encodeChannelData(channel: u16, payload: []const u8, out: []u8): usize? {
    if (channel < CHANNEL_MIN || channel > CHANNEL_MAX) {
        return null;
    }
    if (payload.len > 65535) {
        return null;
    }
    const total = CHANNEL_DATA_HEADER_SIZE + payload.len;
    if (out.len < total) {
        return null;
    }
    writeU16Be(out, 0, channel);
    writeU16Be(out, 2, payload.len as u16);
    let i: usize = 0;
    while (i < payload.len) {
        out[CHANNEL_DATA_HEADER_SIZE + i] = payload[i];
        i += 1;
    }
    return total;
}
