// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Siemens S7comm / TPKT RFC 1006 / COTP ISO 8073 Codec)

pub const S7_TCP_PORT: u16 = 102;
pub const TPKT_VERSION: u8 = 3;
pub const TPKT_HEADER_SIZE: usize = 4;
pub const S7_PROTOCOL_ID: u8 = 0x32;
pub const S7_HEADER_SIZE: usize = 10;
pub const S7_ACK_HEADER_SIZE: usize = 12;

// COTP PDU Types (ISO 8073, upper 4 bits)
pub const COTP_CR: u8 = 0xE0;
pub const COTP_CC: u8 = 0xD0;
pub const COTP_DR: u8 = 0x80;
pub const COTP_DT: u8 = 0xF0;

// S7comm ROSCTR (Remote Operating Service Control)
pub const ROSCTR_JOB: u8 = 1;
pub const ROSCTR_ACK: u8 = 2;
pub const ROSCTR_ACK_DATA: u8 = 3;
pub const ROSCTR_USERDATA: u8 = 7;

// S7comm Function Codes
pub const FUNC_READ_VAR: u8 = 0x04;
pub const FUNC_WRITE_VAR: u8 = 0x05;
pub const FUNC_SETUP_COMM: u8 = 0xF0;

// S7comm Memory Areas
pub const AREA_SYS_INFO: u8 = 0x03;
pub const AREA_INPUTS: u8 = 0x81;
pub const AREA_OUTPUTS: u8 = 0x82;
pub const AREA_FLAGS: u8 = 0x83;
pub const AREA_DB: u8 = 0x84;
pub const AREA_COUNTERS: u8 = 0x1C;
pub const AREA_TIMERS: u8 = 0x1D;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

/// Fast prefix check for RFC 1006 TPKT frames.
pub fn isTpkt(data: []const u8): bool {
    if (data.len < TPKT_HEADER_SIZE) {
        return false;
    }
    if (data[0] != TPKT_VERSION) {
        return false;
    }
    const len = readU16Be(data, 2) as usize;
    return len >= TPKT_HEADER_SIZE && len <= data.len;
}

/// Parsed RFC 1006 TPKT frame.
pub struct TpktFrame {
    version: u8,
    length: u16,
    payload: []const u8,
}

/// Strictly parses an RFC 1006 TPKT frame (`version == 3`, `4 <= length <= data.len`).
pub fn parseTpkt(data: []const u8): TpktFrame? {
    if (data.len < TPKT_HEADER_SIZE) {
        return null;
    }
    const version = data[0];
    if (version != TPKT_VERSION) {
        return null;
    }
    const length = readU16Be(data, 2);
    const len_usize = length as usize;
    if (len_usize < TPKT_HEADER_SIZE || len_usize > data.len) {
        return null;
    }
    return TpktFrame{
        .version = version,
        .length = length,
        .payload = data[TPKT_HEADER_SIZE..len_usize],
    };
}

/// Zero-alloc encoder for an RFC 1006 TPKT frame.
pub fn encodeTpkt(payload: []const u8, out: []u8): usize? {
    const total = TPKT_HEADER_SIZE + payload.len;
    if (total > 65535 || out.len < total) {
        return null;
    }
    out[0] = TPKT_VERSION;
    out[1] = 0x00;
    writeU16Be(out, 2, total as u16);
    let i: usize = 0;
    while (i < payload.len) {
        out[TPKT_HEADER_SIZE + i] = payload[i];
        i += 1;
    }
    return total;
}

/// Parsed ISO 8073 COTP header and payload.
pub struct CotpHeader {
    li: u8,
    pdu_type: u8,
    tpdu_nr: u8,
    eot: bool,
    payload: []const u8,
}

/// Strictly parses a COTP header (supports 3-byte DT Data Transfer and variable-length CR/CC/DR headers).
pub fn parseCotp(data: []const u8): CotpHeader? {
    if (data.len < 2) {
        return null;
    }
    const li = data[0];
    const hdr_len = 1 + (li as usize);
    if (hdr_len > data.len || hdr_len < 2) {
        return null;
    }
    const pdu_type = data[1] & 0xF0;

    if (pdu_type == COTP_DT) {
        if (hdr_len < 3) {
            return null;
        }
        const nr_byte = data[2];
        return CotpHeader{
            .li = li,
            .pdu_type = pdu_type,
            .tpdu_nr = nr_byte & 0x7F,
            .eot = (nr_byte & 0x80) != 0,
            .payload = data[hdr_len..],
        };
    }

    if (pdu_type == COTP_CR || pdu_type == COTP_CC || pdu_type == COTP_DR) {
        return CotpHeader{
            .li = li,
            .pdu_type = pdu_type,
            .tpdu_nr = 0,
            .eot = false,
            .payload = data[hdr_len..],
        };
    }

    return null;
}

/// Zero-alloc encoder for a 3-byte COTP DT (Data Transfer) frame + payload.
pub fn encodeCotpDt(payload: []const u8, eot: bool, tpdu_nr: u8, out: []u8): usize? {
    const total = 3 + payload.len;
    if (out.len < total) {
        return null;
    }
    out[0] = 2; // LI = 2 (total 3-byte header)
    out[1] = COTP_DT;
    out[2] = (tpdu_nr & 0x7F) | (if (eot) 0x80 else 0x00);
    let i: usize = 0;
    while (i < payload.len) {
        out[3 + i] = payload[i];
        i += 1;
    }
    return total;
}

/// Parsed S7comm header (10 bytes for Job/Userdata, 12 bytes for Ack/Ack_Data).
pub struct S7Header {
    rosctr: u8,
    redundancy_id: u16,
    pdu_ref: u16,
    param_len: u16,
    data_len: u16,
    error_class: u8,
    error_code: u8,
    has_error_fields: bool,
}

/// Parsed S7comm packet (header + parameter section + data section).
pub struct S7Packet {
    header: S7Header,
    params: []const u8,
    data: []const u8,

    fn funcCode(): u8? {
        if (_.params.len >= 1) {
            return _.params[0];
        }
        return null;
    }
}

fn isValidRosctr(rosctr: u8): bool {
    return rosctr == ROSCTR_JOB || rosctr == ROSCTR_ACK || rosctr == ROSCTR_ACK_DATA || rosctr == ROSCTR_USERDATA;
}

/// Fast prefix check for S7comm packets (`protocol_id == 0x32` and valid ROSCTR).
pub fn isS7(data: []const u8): bool {
    if (data.len < S7_HEADER_SIZE) {
        return false;
    }
    if (data[0] != S7_PROTOCOL_ID) {
        return false;
    }
    return isValidRosctr(data[1]);
}

/// Strictly parses an S7comm packet (`protocol_id == 0x32`, valid ROSCTR, bounds-checked params & data).
pub fn parseS7(data: []const u8): S7Packet? {
    if (data.len < S7_HEADER_SIZE) {
        return null;
    }
    if (data[0] != S7_PROTOCOL_ID) {
        return null;
    }
    const rosctr = data[1];
    if (!isValidRosctr(rosctr)) {
        return null;
    }

    const has_err = (rosctr == ROSCTR_ACK || rosctr == ROSCTR_ACK_DATA);
    const hdr_len: usize = if (has_err) S7_ACK_HEADER_SIZE else S7_HEADER_SIZE;
    if (data.len < hdr_len) {
        return null;
    }

    const redundancy_id = readU16Be(data, 2);
    const pdu_ref = readU16Be(data, 4);
    const param_len = readU16Be(data, 6);
    const data_len = readU16Be(data, 8);
    const error_class: u8 = if (has_err) data[10] else 0;
    const error_code: u8 = if (has_err) data[11] else 0;

    const p_len = param_len as usize;
    const d_len = data_len as usize;
    if (hdr_len + p_len + d_len > data.len) {
        return null;
    }

    const params_start = hdr_len;
    const data_start = params_start + p_len;
    return S7Packet{
        .header = S7Header{
            .rosctr = rosctr,
            .redundancy_id = redundancy_id,
            .pdu_ref = pdu_ref,
            .param_len = param_len,
            .data_len = data_len,
            .error_class = error_class,
            .error_code = error_code,
            .has_error_fields = has_err,
        },
        .params = data[params_start..data_start],
        .data = data[data_start .. data_start + d_len],
    };
}

/// One-step decapsulation: TPKT -> COTP DT -> S7Packet.
pub fn parseS7OverTpkt(data: []const u8): S7Packet? {
    const tpkt = parseTpkt(data) ?? return null;
    const cotp = parseCotp(tpkt.payload) ?? return null;
    if (cotp.pdu_type != COTP_DT) {
        return null;
    }
    return parseS7(cotp.payload);
}

/// Decoded 12-byte S7-Any address specification item (`0x12, 0x0A, 0x10, ...`).
pub struct S7AnyItem {
    transport_size: u8,
    length: u16,
    db_number: u16,
    area: u8,
    byte_addr: u32,
    bit_offset: u8,
}

/// Zero-copy iterator over 12-byte S7-Any items in a Read/Write Var parameter section.
pub struct S7AnyItemIter {
    data: []const u8,
    offset: usize,
    remaining: u8,
    failed: bool,

    fn init(items_bytes: []const u8, count: u8): S7AnyItemIter {
        return S7AnyItemIter{
            .data = items_bytes,
            .offset = 0,
            .remaining = count,
            .failed = false,
        };
    }

    /// Initializes an iterator directly from `S7Packet.params` of a Read/Write Var Job.
    fn initFromParams(params: []const u8): S7AnyItemIter? {
        if (params.len < 2) {
            return null;
        }
        const fc = params[0];
        if (fc != FUNC_READ_VAR && fc != FUNC_WRITE_VAR) {
            return null;
        }
        const count = params[1];
        return S7AnyItemIter{
            .data = params[2..],
            .offset = 0,
            .remaining = count,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): S7AnyItem? {
        if (_.failed || _.remaining == 0) {
            return null;
        }
        if (_.offset + 12 > _.data.len) {
            _.failed = true;
            return null;
        }
        const off = _.offset;
        // S7-Any prefix: var_spec=0x12, addr_len=0x0A, syntax_id=0x10
        if (_.data[off] != 0x12 || _.data[off + 1] != 0x0A || _.data[off + 2] != 0x10) {
            _.failed = true;
            return null;
        }
        const transport_size = _.data[off + 3];
        const length = readU16Be(_.data, off + 4);
        const db_number = readU16Be(_.data, off + 6);
        const area = _.data[off + 8];
        const raw_bit_addr: u32 = ((_.data[off + 9] as u32) << 16) | ((_.data[off + 10] as u32) << 8) | (_.data[off + 11] as u32);
        const byte_addr = raw_bit_addr >> 3;
        const bit_offset = (raw_bit_addr & 0x07) as u8;

        _.offset += 12;
        _.remaining -= 1;

        return S7AnyItem{
            .transport_size = transport_size,
            .length = length,
            .db_number = db_number,
            .area = area,
            .byte_addr = byte_addr,
            .bit_offset = bit_offset,
        };
    }
}
