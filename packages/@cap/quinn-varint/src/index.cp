// @cap/quinn-varint — Pure CP zero-allocation RFC 9000 QUIC VarInt & Long Header parser
// Parity with Rust `quinn-proto` (RFC 9000 Section 16 & Section 17).

pub const MAX_VARINT: u64 = 4611686018427387903;

pub const PKT_INITIAL: u8 = 0;
pub const PKT_ZERO_RTT: u8 = 1;
pub const PKT_HANDSHAKE: u8 = 2;
pub const PKT_RETRY: u8 = 3;

pub const QUIC_VERSION_1: u32 = 1;

pub struct QuicVarint {
    pub val: u64,
    pub bytes_read: usize,
    pub ok: bool,
}

pub struct QuicLongHeader {
    pub is_long: bool,
    pub packet_type: u8,
    pub version: u32,
    pub dcid_len: u8,
    pub scid_len: u8,
    pub header_len: usize,
    pub ok: bool,
}

pub fn encodedLen(val: u64): usize {
    if (val <= 63) {
        return 1;
    }
    if (val <= 16383) {
        return 2;
    }
    if (val <= 1073741823) {
        return 4;
    }
    if (val <= MAX_VARINT) {
        return 8;
    }
    return 0;
}

pub fn encodeVarint(val: u64, out: []u8): usize {
    let n = encodedLen(val);
    if (n == 0 || out.len < n) {
        return 0;
    }
    if (n == 1) {
        out[0] = val as u8;
        return 1;
    }
    if (n == 2) {
        out[0] = (0x40 | ((val >> 8) & 0x3F)) as u8;
        out[1] = (val & 0xFF) as u8;
        return 2;
    }
    if (n == 4) {
        out[0] = (0x80 | ((val >> 24) & 0x3F)) as u8;
        out[1] = ((val >> 16) & 0xFF) as u8;
        out[2] = ((val >> 8) & 0xFF) as u8;
        out[3] = (val & 0xFF) as u8;
        return 4;
    }
    out[0] = (0xC0 | ((val >> 56) & 0x3F)) as u8;
    out[1] = ((val >> 48) & 0xFF) as u8;
    out[2] = ((val >> 40) & 0xFF) as u8;
    out[3] = ((val >> 32) & 0xFF) as u8;
    out[4] = ((val >> 24) & 0xFF) as u8;
    out[5] = ((val >> 16) & 0xFF) as u8;
    out[6] = ((val >> 8) & 0xFF) as u8;
    out[7] = (val & 0xFF) as u8;
    return 8;
}

pub fn decodeVarint(buf: []u8): QuicVarint {
    if (buf.len == 0) {
        return QuicVarint { val: 0, bytes_read: 0, ok: false };
    }
    let first = buf[0];
    let prefix = first >> 6;
    let n: usize = 1;
    if (prefix == 1) {
        n = 2;
    } else if (prefix == 2) {
        n = 4;
    } else if (prefix == 3) {
        n = 8;
    }

    if (buf.len < n) {
        return QuicVarint { val: 0, bytes_read: 0, ok: false };
    }

    let val: u64 = (first & 0x3F) as u64;
    let i: usize = 1;
    while (i < n) {
        val = (val << 8) | (buf[i] as u64);
        i = i + 1;
    }
    return QuicVarint { val: val, bytes_read: n, ok: true };
}

fn qvInvalidHeader(): QuicLongHeader {
    return QuicLongHeader {
        is_long: false,
        packet_type: 0,
        version: 0,
        dcid_len: 0,
        scid_len: 0,
        header_len: 0,
        ok: false,
    };
}

pub fn parseLongHeader(pkt: []u8): QuicLongHeader {
    if (pkt.len == 0) {
        return qvInvalidHeader();
    }
    let b0 = pkt[0];
    let is_long = (b0 & 0x80) != 0;
    let fixed_bit = (b0 & 0x40) != 0;

    // Short Header (1-RTT packet, RFC 9000 Section 17.3)
    if (!is_long) {
        if (!fixed_bit) {
            return qvInvalidHeader();
        }
        return QuicLongHeader {
            is_long: false,
            packet_type: 0,
            version: 0,
            dcid_len: 0,
            scid_len: 0,
            header_len: 1,
            ok: true,
        };
    }

    // Long Header requires at least 7 bytes (flags + 4B version + 1B dcid_len + 1B scid_len)
    if (pkt.len < 7) {
        return qvInvalidHeader();
    }

    let version: u32 = ((pkt[1] as u32) << 24)
        | ((pkt[2] as u32) << 16)
        | ((pkt[3] as u32) << 8)
        | (pkt[4] as u32);

    // Non-Version-Negotiation packets require Fixed Bit == 1
    if (version != 0 && !fixed_bit) {
        return qvInvalidHeader();
    }

    let packet_type: u8 = (b0 >> 4) & 0x03;
    let dcid_len: u8 = pkt[5];
    let scid_len_pos: usize = (6 as usize) + (dcid_len as usize);
    if (scid_len_pos >= pkt.len) {
        return qvInvalidHeader();
    }
    let scid_len: u8 = pkt[scid_len_pos];

    // RFC 9000 Section 17.2: QUIC v1 connection IDs are at most 20 bytes
    if (version == QUIC_VERSION_1 && (dcid_len > 20 || scid_len > 20)) {
        return qvInvalidHeader();
    }

    let header_len: usize = scid_len_pos + 1 + (scid_len as usize);
    if (pkt.len < header_len) {
        return qvInvalidHeader();
    }

    return QuicLongHeader {
        is_long: true,
        packet_type: packet_type,
        version: version,
        dcid_len: dcid_len,
        scid_len: scid_len,
        header_len: header_len,
        ok: true,
    };
}

export {
    MAX_VARINT,
    PKT_INITIAL,
    PKT_ZERO_RTT,
    PKT_HANDSHAKE,
    PKT_RETRY,
    QUIC_VERSION_1,
    QuicVarint,
    QuicLongHeader,
    encodedLen,
    encodeVarint,
    decodeVarint,
    parseLongHeader
};
