// Origin: Pure-CP Stdlib (Zero-Alloc ISO/IEC 14543-3-2 KNXnet/IP, cEMI & DPT9 Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (ISO/IEC 14543-3-2 KNXnet/IP & cEMI)
// ---------------------------------------------------------------------------

pub const KNX_UDP_PORT: u16 = 3671;
pub const KNX_HEADER_SIZE: usize = 6;
pub const KNX_PROTOCOL_V10: u8 = 0x10;
pub const KNX_HPAI_SIZE: usize = 8;
pub const KNX_CONN_HEADER_SIZE: usize = 4;

// KNXnet/IP Service Types
pub const SVC_SEARCH_REQ: u16 = 0x0201;
pub const SVC_SEARCH_RESP: u16 = 0x0202;
pub const SVC_DESCRIPTION_REQ: u16 = 0x0203;
pub const SVC_DESCRIPTION_RESP: u16 = 0x0204;
pub const SVC_CONNECT_REQ: u16 = 0x0205;
pub const SVC_CONNECT_RESP: u16 = 0x0206;
pub const SVC_CONNECTIONSTATE_REQ: u16 = 0x0207;
pub const SVC_CONNECTIONSTATE_RESP: u16 = 0x0208;
pub const SVC_DISCONNECT_REQ: u16 = 0x0209;
pub const SVC_DISCONNECT_RESP: u16 = 0x020A;
pub const SVC_TUNNELLING_REQ: u16 = 0x0420;
pub const SVC_TUNNELLING_ACK: u16 = 0x0421;
pub const SVC_ROUTING_INDICATION: u16 = 0x0530;

// HPAI Host Protocol Codes
pub const HOST_PROTO_IPV4_UDP: u8 = 0x01;
pub const HOST_PROTO_IPV4_TCP: u8 = 0x02;

// cEMI Message Codes
pub const CEMI_L_DATA_REQ: u8 = 0x11;
pub const CEMI_L_DATA_IND: u8 = 0x29;
pub const CEMI_L_DATA_CON: u8 = 0x2E;

// 10-bit APCI Command Codes
pub const APCI_GROUP_READ: u16 = 0x000;
pub const APCI_GROUP_RESPONSE: u16 = 0x040;
pub const APCI_GROUP_WRITE: u16 = 0x080;

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn writeU16Be(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// KNXnet/IP Packet, HPAI & Connection Header Models
// ---------------------------------------------------------------------------

/// Decoded 6-byte KNXnet/IP header.
pub struct KnxHeader {
    header_len: u8,
    protocol_version: u8,
    service_type: u16,
    total_len: u16,
}

/// Zero-copy view of a parsed KNXnet/IP packet.
pub struct KnxPacket {
    header: KnxHeader,
    body: []const u8,
}

/// Decoded 8-byte Host Protocol Address Information (HPAI).
pub struct KnxHpai {
    host_protocol: u8,
    ip: [4]u8,
    port: u16,
}

/// Decoded 4-byte KNXnet/IP Connection Header (used in Tunnelling Req/Ack).
pub struct KnxConnHeader {
    channel_id: u8,
    seq_counter: u8,
    status: u8,
}

// ---------------------------------------------------------------------------
// KNX Individual (Physical) & 3-Level Group Addresses
// ---------------------------------------------------------------------------

/// KNX physical/individual address (`area.line.device`: 4 / 4 / 8 bits).
pub struct KnxIndividualAddr {
    area: u8,
    line: u8,
    device: u8,
}

/// KNX 3-level group address (`main/middle/sub`: 5 / 3 / 8 bits).
pub struct KnxGroupAddr {
    main: u8,
    middle: u8,
    sub: u8,
}

pub fn decodeIndividualAddr(raw: u16): KnxIndividualAddr {
    return KnxIndividualAddr {
        area: ((raw >> 12) & 0x0F) as u8,
        line: ((raw >> 8) & 0x0F) as u8,
        device: (raw & 0xFF) as u8,
    };
}

pub fn encodeIndividualAddr(area: u8, line: u8, device: u8): u16 {
    return (((area as u16) & 0x0F) << 12) | (((line as u16) & 0x0F) << 8) | (device as u16);
}

pub fn decodeGroupAddr(raw: u16): KnxGroupAddr {
    return KnxGroupAddr {
        main: ((raw >> 11) & 0x1F) as u8,
        middle: ((raw >> 8) & 0x07) as u8,
        sub: (raw & 0xFF) as u8,
    };
}

pub fn encodeGroupAddr(main: u8, middle: u8, sub: u8): u16 {
    return (((main as u16) & 0x1F) << 11) | (((middle as u16) & 0x07) << 8) | (sub as u16);
}

// ---------------------------------------------------------------------------
// cEMI L_Data Frame
// ---------------------------------------------------------------------------

/// Zero-copy view of a parsed KNX cEMI L_Data frame.
pub struct KnxCemi {
    msg_code: u8,
    add_info: []const u8,
    ctrl1: u8,
    ctrl2: u8,
    src_addr: u16,
    dst_addr: u16,
    apci: u16,
    short_data: u8,
    payload: []const u8,

    /// True if destination address is a Group Address (`ctrl2` bit 7 set).
    fn isGroupAddress(): bool {
        return (_.ctrl2 & 0x80) != 0;
    }

    /// True if standard frame (`ctrl1` bit 7 set); false for extended frame.
    fn isStandardFrame(): bool {
        return (_.ctrl1 & 0x80) != 0;
    }

    /// Returns 2-bit KNX transmission priority (`0=System, 1=Normal, 2=Urgent, 3=Low`).
    fn priority(): u8 {
        return (_.ctrl1 >> 2) & 0x03;
    }
}

// ---------------------------------------------------------------------------
// KNXnet/IP, HPAI, Connection Header & cEMI Parsers / Encoders
// ---------------------------------------------------------------------------

/// Fast-path validation of a KNXnet/IP packet header (`header_len == 6`, `v1.0`, `6 <= total_len <= data.len`).
pub fn isKnx(data: []const u8): bool {
    if (data.len < KNX_HEADER_SIZE) {
        return false;
    }
    if (data[0] != (KNX_HEADER_SIZE as u8) || data[1] != KNX_PROTOCOL_V10) {
        return false;
    }
    const total_len = readU16Be(data, 4) as usize;
    if (total_len < KNX_HEADER_SIZE || total_len > data.len) {
        return false;
    }
    return true;
}

/// Parses a KNXnet/IP packet into its 6-byte header and zero-copy body slice.
pub fn parseKnx(data: []const u8): KnxPacket? {
    if (!isKnx(data)) {
        return null;
    }
    const service_type = readU16Be(data, 2);
    const total_len = readU16Be(data, 4);
    return KnxPacket {
        header: KnxHeader {
            header_len: data[0],
            protocol_version: data[1],
            service_type: service_type,
            total_len: total_len,
        },
        body: data[KNX_HEADER_SIZE..total_len as usize],
    };
}

/// Encodes a 6-byte KNXnet/IP header into `out`.
/// Returns `6` on success, or `null` if `6 + body_len > 65535` or `out.len < 6`.
pub fn encodeKnxHeader(service_type: u16, body_len: usize, out: []u8): usize? {
    if (out.len < KNX_HEADER_SIZE) {
        return null;
    }
    const total = KNX_HEADER_SIZE + body_len;
    if (total > 65535) {
        return null;
    }
    out[0] = KNX_HEADER_SIZE as u8;
    out[1] = KNX_PROTOCOL_V10;
    writeU16Be(out, 2, service_type);
    writeU16Be(out, 4, total as u16);
    return KNX_HEADER_SIZE;
}

/// Parses an 8-byte KNXnet/IP HPAI structure.
pub fn parseKnxHpai(data: []const u8): KnxHpai? {
    if (data.len < KNX_HPAI_SIZE || data[0] != (KNX_HPAI_SIZE as u8)) {
        return null;
    }
    const proto = data[1];
    if (proto != HOST_PROTO_IPV4_UDP && proto != HOST_PROTO_IPV4_TCP) {
        return null;
    }
    return KnxHpai {
        host_protocol: proto,
        ip: [data[2], data[3], data[4], data[5]],
        port: readU16Be(data, 6),
    };
}

/// Parses a 4-byte KNXnet/IP Connection Header.
pub fn parseKnxConnHeader(data: []const u8): KnxConnHeader? {
    if (data.len < KNX_CONN_HEADER_SIZE || data[0] != (KNX_CONN_HEADER_SIZE as u8)) {
        return null;
    }
    return KnxConnHeader {
        channel_id: data[1],
        seq_counter: data[2],
        status: data[3],
    };
}

/// Parses a KNX cEMI L_Data frame (e.g. `L_Data.req`, `L_Data.ind`, `L_Data.con`).
pub fn parseKnxCemi(data: []const u8): KnxCemi? {
    if (data.len < 2) {
        return null;
    }
    const msg_code = data[0];
    const add_len = data[1] as usize;
    const base = 2 + add_len;
    // Minimum remaining bytes: ctrl1(1) + ctrl2(1) + src(2) + dst(2) + npdu_len(1) + tpci/apci(2) = 9
    if (base + 9 > data.len) {
        return null;
    }
    const add_info = data[2..base];
    const ctrl1 = data[base];
    const ctrl2 = data[base + 1];
    const src_addr = readU16Be(data, base + 2);
    const dst_addr = readU16Be(data, base + 4);
    const npdu_len = data[base + 6] as usize;
    if (npdu_len < 1) {
        return null;
    }
    const apdu_end = base + 7 + npdu_len + 1;
    if (apdu_end > data.len) {
        return null;
    }
    const tpci_hi = data[base + 7];
    const apci_lo = data[base + 8];
    const apci: u16 = (((tpci_hi & 0x03) as u16) << 8) | ((apci_lo & 0xC0) as u16);
    const short_data = apci_lo & 0x3F;
    const payload = data[base + 9..apdu_end];

    return KnxCemi {
        msg_code: msg_code,
        add_info: add_info,
        ctrl1: ctrl1,
        ctrl2: ctrl2,
        src_addr: src_addr,
        dst_addr: dst_addr,
        apci: apci,
        short_data: short_data,
        payload: payload,
    };
}

// ---------------------------------------------------------------------------
// KNX DPT9 2-Byte Floating-Point Codec
// ---------------------------------------------------------------------------

/// Decodes a KNX DPT9 2-byte float (`MEEE EMMM MMMM MMMM`) to `f64`:
/// `0.01 * signed_11bit_mantissa * 2^exp_4bit`.
pub fn decodeDpt9(raw: u16): f64 {
    const sign = (raw >> 15) & 0x01;
    const exp = ((raw >> 11) & 0x0F) as u5;
    let mant: i32 = (raw & 0x07FF) as i32;
    if (sign != 0) {
        mant = mant - 2048;
    }
    const scale = ((1 as u32) << exp) as f64;
    return 0.01 * (mant as f64) * scale;
}
