// Origin: Pure-CP Stdlib (Zero-Alloc RFC 2661 L2TPv2 & RFC 3931 L2TPv3 Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation L2TPv2 & L2TPv3 wire protocol parser:
// L2TPv2 Control/Data headers (L/S/O/P flags), L2TPv3 Control-over-UDP header,
// and zero-allocation 6-byte header / 10-bit length AVP iterator.

/// Standard L2TP UDP port (`1701`).
pub const L2TP_UDP_PORT: u16 = 1701;

/// L2TPv3 IP protocol number (`115`, RFC 3931).
pub const L2TP_IP_PROTO: u8 = 115;

/// L2TP version 2 (`2`, RFC 2661).
pub const L2TP_V2: u8 = 2;

/// L2TP version 3 (`3`, RFC 3931).
pub const L2TP_V3: u8 = 3;

/// Fixed L2TP AVP header size in bytes (`6`).
pub const L2TP_AVP_HEADER_SIZE: usize = 6;

// L2TP Header Flag Bitmasks (RFC 2661 Section 3.1)
pub const FLAG_TYPE_CTRL: u16 = 0x8000;
pub const FLAG_LENGTH: u16 = 0x4000;
pub const FLAG_SEQUENCE: u16 = 0x0800;
pub const FLAG_OFFSET: u16 = 0x0200;
pub const FLAG_PRIORITY: u16 = 0x0100;

// Common L2TPv2 AVP Attribute Types (RFC 2661 Section 4.4)
pub const AVP_MESSAGE_TYPE: u16 = 0;
pub const AVP_RESULT_CODE: u16 = 1;
pub const AVP_PROTOCOL_VERSION: u16 = 2;
pub const AVP_FRAMING_CAPS: u16 = 3;
pub const AVP_BEARER_CAPS: u16 = 4;
pub const AVP_HOST_NAME: u16 = 7;
pub const AVP_VENDOR_NAME: u16 = 8;
pub const AVP_ASSIGNED_TUNNEL_ID: u16 = 9;
pub const AVP_RECEIVE_WINDOW_SIZE: u16 = 10;
pub const AVP_ASSIGNED_SESSION_ID: u16 = 14;
pub const AVP_CALL_SERIAL_NUMBER: u16 = 15;

/// Zero-copy decoded L2TP Attribute-Value Pair (AVP).
pub struct L2tpAvp {
    mandatory: bool = false,
    hidden: bool = false,
    length: u16 = 6,
    vendor_id: u16 = 0,
    attr_type: u16 = 0,
    value: []const u8 = "",
}

/// Zero-allocation iterator over L2TP AVPs (`6B header + variable value`).
pub struct L2tpAvpIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(avp_bytes: []const u8): L2tpAvpIter {
        return L2tpAvpIter {
            data: avp_bytes,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): L2tpAvp? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data.len - _.pos;
        if (rem < L2TP_AVP_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const b01 = readBeU16(_.data, _.pos);
        const mandatory = (b01 & 0x8000) != 0;
        const hidden = (b01 & 0x4000) != 0;
        const length: u16 = b01 & 0x03FF;
        const len_usize = length as usize;
        if (len_usize < L2TP_AVP_HEADER_SIZE || len_usize > rem) {
            _.failed = true;
            return null;
        }
        const vendor_id = readBeU16(_.data, _.pos + 2);
        const attr_type = readBeU16(_.data, _.pos + 4);
        const value = _.data[(_.pos + L2TP_AVP_HEADER_SIZE)..(_.pos + len_usize)];
        _.pos += len_usize;

        return L2tpAvp {
            mandatory: mandatory,
            hidden: hidden,
            length: length,
            vendor_id: vendor_id,
            attr_type: attr_type,
            value: value,
        };
    }
}

/// Zero-copy decoded RFC 2661 L2TPv2 Control or Data packet.
pub struct L2tpv2Packet {
    is_control: bool = false,
    has_length: bool = false,
    has_seq: bool = false,
    has_offset: bool = false,
    is_priority: bool = false,
    length: u16? = null,
    tunnel_id: u16 = 0,
    session_id: u16 = 0,
    ns: u16? = null,
    nr: u16? = null,
    offset_size: u16? = null,
    payload: []const u8 = "",

    /// Returns a zero-allocation AVP iterator when `is_control == true` (empty iterator otherwise).
    fn avps(): L2tpAvpIter {
        if (!_.is_control) {
            return L2tpAvpIter.init("");
        }
        return L2tpAvpIter.init(_.payload);
    }
}

/// Zero-copy decoded RFC 3931 L2TPv3 Control-over-UDP packet header and AVP bytes.
pub struct L2tpv3ControlHeader {
    length: u16 = 12,
    cc_id: u32 = 0,
    ns: u16 = 0,
    nr: u16 = 0,
    avp_bytes: []const u8 = "",

    /// Returns a zero-allocation iterator over the L2TPv3 control message AVPs.
    fn avps(): L2tpAvpIter {
        return L2tpAvpIter.init(_.avp_bytes);
    }
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

// ---------------------------------------------------------------------------
// Public L2TPv2 & L2TPv3 Parsers
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 2661 L2TPv2 Control or Data packet.
/// Validates `version == 2`, enforces `L=1, S=1, O=0, P=0` on Control messages,
/// and decodes optional `length`, `ns/nr`, and `offset_size` + offset pad safely.
pub fn parseL2tpv2(data: []const u8): L2tpv2Packet? {
    if (data.len < 6) {
        return null;
    }
    const flags_ver = readBeU16(data, 0);
    const ver = (flags_ver & 0x000F) as u8;
    if (ver != L2TP_V2) {
        return null;
    }
    const is_control = (flags_ver & FLAG_TYPE_CTRL) != 0;
    const has_length = (flags_ver & FLAG_LENGTH) != 0;
    const has_seq = (flags_ver & FLAG_SEQUENCE) != 0;
    const has_offset = (flags_ver & FLAG_OFFSET) != 0;
    const is_priority = (flags_ver & FLAG_PRIORITY) != 0;

    if (is_control) {
        if (!has_length || !has_seq || has_offset || is_priority) {
            return null;
        }
    }

    let pos: usize = 2;
    let pkt_end: usize = data.len;
    let length: u16? = null;
    if (has_length) {
        if (data.len - pos < 2) {
            return null;
        }
        const len_val = readBeU16(data, pos);
        const len_usize = len_val as usize;
        if (len_usize > data.len) {
            return null;
        }
        length = len_val;
        pkt_end = len_usize;
        pos += 2;
    }

    if (pkt_end < pos || pkt_end - pos < 4) {
        return null;
    }
    const tunnel_id = readBeU16(data, pos);
    const session_id = readBeU16(data, pos + 2);
    pos += 4;

    let ns: u16? = null;
    let nr: u16? = null;
    if (has_seq) {
        if (pkt_end - pos < 4) {
            return null;
        }
        ns = readBeU16(data, pos);
        nr = readBeU16(data, pos + 2);
        pos += 4;
    }

    let offset_size: u16? = null;
    if (has_offset) {
        if (pkt_end - pos < 2) {
            return null;
        }
        const off_sz = readBeU16(data, pos);
        offset_size = off_sz;
        pos += 2;
        if (pkt_end - pos < (off_sz as usize)) {
            return null;
        }
        pos += off_sz as usize;
    }

    return L2tpv2Packet {
        is_control: is_control,
        has_length: has_length,
        has_seq: has_seq,
        has_offset: has_offset,
        is_priority: is_priority,
        length: length,
        tunnel_id: tunnel_id,
        session_id: session_id,
        ns: ns,
        nr: nr,
        offset_size: offset_size,
        payload: data[pos..pkt_end],
    };
}

/// Strictly parses an RFC 3931 L2TPv3 Control-over-UDP packet (`session_id == 0`,
/// `T=1, L=1, S=1, ver=3`, `12 <= length <= data.len - 4`).
pub fn parseL2tpv3Control(data: []const u8): L2tpv3ControlHeader? {
    if (data.len < 16) {
        return null;
    }
    // RFC 3931 Section 4.1.2.1: leading 4-byte Session ID must be 0 for Control over UDP
    if (readBeU32(data, 0) != 0) {
        return null;
    }
    const flags_ver = readBeU16(data, 4);
    const ver = (flags_ver & 0x000F) as u8;
    if (ver != L2TP_V3) {
        return null;
    }
    const req_flags = FLAG_TYPE_CTRL | FLAG_LENGTH | FLAG_SEQUENCE;
    if ((flags_ver & req_flags) != req_flags) {
        return null;
    }
    const length = readBeU16(data, 6);
    const len_usize = length as usize;
    if (len_usize < 12 || len_usize > data.len - 4) {
        return null;
    }
    const cc_id = readBeU32(data, 8);
    const ns = readBeU16(data, 12);
    const nr = readBeU16(data, 14);
    return L2tpv3ControlHeader {
        length: length,
        cc_id: cc_id,
        ns: ns,
        nr: nr,
        avp_bytes: data[16..(4 + len_usize)],
    };
}

/// Returns `true` if `data` is a structurally valid L2TPv2 packet or L2TPv3 Control-over-UDP packet.
pub fn isL2tp(data: []const u8): bool {
    if (v2 := parseL2tpv2(data)) {
        * = v2;
        return true;
    }
    if (v3 := parseL2tpv3Control(data)) {
        * = v3;
        return true;
    }
    return false;
}
