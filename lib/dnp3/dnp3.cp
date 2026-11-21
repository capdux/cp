// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IEEE 1815 DNP3 Wire Protocol & CRC-16 Codec)

// ---------------------------------------------------------------------------
// Protocol Constants (IEEE 1815 DNP3)
// ---------------------------------------------------------------------------

pub const DNP3_TCP_PORT: u16 = 20000;
pub const DNP3_TLS_PORT: u16 = 19999;
pub const DNP3_START_BYTES: u16 = 0x0564;
pub const DNP3_HEADER_SIZE: usize = 10;
pub const DNP3_BLOCK_SIZE: usize = 16;
pub const DNP3_MAX_PAYLOAD_LEN: usize = 250;
pub const DNP3_CRC_POLY: u16 = 0xA6BC;

// Data Link Control Flags
pub const DL_DIR: u8 = 0x80;
pub const DL_PRM: u8 = 0x40;
pub const DL_FCB_DFC: u8 = 0x20;
pub const DL_FCV: u8 = 0x10;

// Application Function Codes
pub const FC_CONFIRM: u8 = 0;
pub const FC_READ: u8 = 1;
pub const FC_WRITE: u8 = 2;
pub const FC_SELECT: u8 = 3;
pub const FC_OPERATE: u8 = 4;
pub const FC_DIRECT_OPERATE: u8 = 5;
pub const FC_COLD_RESTART: u8 = 13;
pub const FC_RESPONSE: u8 = 129;
pub const FC_UNSOLICITED_RESPONSE: u8 = 130;

// ---------------------------------------------------------------------------
// Internal Little-Endian Helpers
// ---------------------------------------------------------------------------

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn writeU16Le(buf: []u8, off: usize, val: u16) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// IEEE 1815 DNP3 CRC-16 (Reflected Poly 0xA6BC, Init 0x0000, Final XOR 0xFFFF)
// ---------------------------------------------------------------------------

/// Computes the IEEE 1815 DNP3 CRC-16 checksum (reflected poly `0xA6BC`, init `0x0000`, final `^ 0xFFFF`).
pub fn dnp3Crc16(data: []const u8): u16 {
    let crc: u16 = 0x0000;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u16);
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 0x0001) != 0) {
                crc = (crc >> 1) ^ DNP3_CRC_POLY;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc ^ 0xFFFF;
}

// ---------------------------------------------------------------------------
// Data Link, Transport & Application Models
// ---------------------------------------------------------------------------

/// Decoded 10-byte DNP3 FT3 Data Link Header.
pub struct Dnp3LinkHeader {
    length: u8,
    ctrl: u8,
    dst: u16,
    src: u16,
    crc: u16,

    /// True if frame originates from Master station (`DL_DIR` bit set).
    fn isFromMaster(): bool {
        return (_.ctrl & DL_DIR) != 0;
    }

    /// True if frame is initiated by Primary station (`DL_PRM` bit set).
    fn isPrimary(): bool {
        return (_.ctrl & DL_PRM) != 0;
    }

    /// Returns the 4-bit Data Link function code (`ctrl & 0x0F`).
    fn funcCode(): u8 {
        return _.ctrl & 0x0F;
    }

    /// Returns the user payload byte count (`length - 5`, excluding block CRCs).
    fn payloadLen(): usize {
        if (_.length < 5) {
            return 0;
        }
        return (_.length - 5) as usize;
    }
}

/// Decoded 1-byte DNP3 Transport Layer header.
pub struct Dnp3TransportHeader {
    fin: bool,
    fir: bool,
    seq: u8,
}

/// Decoded DNP3 Application Layer header and zero-copy object payload slice.
pub struct Dnp3AppHeader {
    fir: bool,
    fin: bool,
    con: bool,
    uns: bool,
    seq: u8,
    func_code: u8,
    iin: u16?,
    objects: []const u8,

    /// Returns true if this is a Response or Unsolicited Response carrying 2-byte IIN.
    fn isResponse(): bool {
        return _.iin != null;
    }
}

// ---------------------------------------------------------------------------
// Data Link Parsing & Zero-Alloc Frame Encoding
// ---------------------------------------------------------------------------

/// Fast-path validation of a DNP3 Data Link header (start bytes `0x05 0x64`, `length >= 5`, valid header CRC-16).
pub fn isDnp3(data: []const u8): bool {
    if (data.len < DNP3_HEADER_SIZE) {
        return false;
    }
    if (data[0] != 0x05 || data[1] != 0x64) {
        return false;
    }
    if (data[2] < 5) {
        return false;
    }
    const expected_crc = dnp3Crc16(data[0..8]);
    const wire_crc = readU16Le(data, 8);
    return wire_crc == expected_crc;
}

/// Parses and CRC-verifies a 10-byte DNP3 Data Link header.
pub fn parseDnp3LinkHeader(data: []const u8): Dnp3LinkHeader? {
    if (!isDnp3(data)) {
        return null;
    }
    return Dnp3LinkHeader {
        length: data[2],
        ctrl: data[3],
        dst: readU16Le(data, 4),
        src: readU16Le(data, 6),
        crc: readU16Le(data, 8),
    };
}

/// Computes the total wire size (including header and 2-byte CRC per 16-byte data block) for `payload_len`.
pub fn dnp3WireFrameSize(payload_len: usize): usize? {
    if (payload_len > DNP3_MAX_PAYLOAD_LEN) {
        return null;
    }
    if (payload_len == 0) {
        return DNP3_HEADER_SIZE;
    }
    const blocks = (payload_len + DNP3_BLOCK_SIZE - 1) / DNP3_BLOCK_SIZE;
    return DNP3_HEADER_SIZE + payload_len + (blocks * 2);
}

/// Validates all 16-byte (or trailing partial) data block CRCs in `frame` and copies
/// the continuous CRC-stripped user payload into `out`.
/// Returns the payload byte count written, or `null` on header/block CRC failure, truncation, or small `out`.
pub fn extractDnp3Payload(frame: []const u8, out: []u8): usize? {
    const hdr = parseDnp3LinkHeader(frame) ?? return null;
    const payload_len = hdr.payloadLen();
    if (out.len < payload_len) {
        return null;
    }
    if (payload_len == 0) {
        return 0;
    }

    let in_pos: usize = DNP3_HEADER_SIZE;
    let out_pos: usize = 0;
    let remaining = payload_len;

    while (remaining > 0) {
        const chunk: usize = if (remaining > DNP3_BLOCK_SIZE) DNP3_BLOCK_SIZE else remaining;
        if (in_pos + chunk + 2 > frame.len) {
            return null;
        }
        const block = frame[in_pos..in_pos + chunk];
        const expected_crc = dnp3Crc16(block);
        const wire_crc = readU16Le(frame, in_pos + chunk);
        if (wire_crc != expected_crc) {
            return null;
        }
        let i: usize = 0;
        while (i < chunk) {
            out[out_pos + i] = block[i];
            i += 1;
        }
        in_pos += chunk + 2;
        out_pos += chunk;
        remaining -= chunk;
    }

    return payload_len;
}

/// Encodes a complete DNP3 Data Link frame (10-byte CRC-protected header + 16-byte CRC-protected data blocks)
/// directly into caller-supplied `out`. Returns total wire bytes written, or `null` if invalid or `out` too small.
pub fn encodeDnp3Frame(
    ctrl: u8,
    dst: u16,
    src: u16,
    payload: []const u8,
    out: []u8,
): usize? {
    const total = dnp3WireFrameSize(payload.len) ?? return null;
    if (out.len < total) {
        return null;
    }

    out[0] = 0x05;
    out[1] = 0x64;
    out[2] = (5 + payload.len) as u8;
    out[3] = ctrl;
    writeU16Le(out, 4, dst);
    writeU16Le(out, 6, src);
    const hdr_crc = dnp3Crc16(out[0..8]);
    writeU16Le(out, 8, hdr_crc);

    let in_pos: usize = 0;
    let out_pos: usize = DNP3_HEADER_SIZE;
    while (in_pos < payload.len) {
        const rem = payload.len - in_pos;
        const chunk: usize = if (rem > DNP3_BLOCK_SIZE) DNP3_BLOCK_SIZE else rem;
        let i: usize = 0;
        while (i < chunk) {
            out[out_pos + i] = payload[in_pos + i];
            i += 1;
        }
        const blk_crc = dnp3Crc16(out[out_pos..out_pos + chunk]);
        writeU16Le(out, out_pos + chunk, blk_crc);
        in_pos += chunk;
        out_pos += chunk + 2;
    }

    return out_pos;
}

// ---------------------------------------------------------------------------
// Transport & Application Layer Parsers
// ---------------------------------------------------------------------------

/// Decodes a 1-byte DNP3 Transport Layer header (`FIN`, `FIR`, 6-bit `SEQ`).
pub fn parseDnp3Transport(b: u8): Dnp3TransportHeader {
    return Dnp3TransportHeader {
        fin: (b & 0x80) != 0,
        fir: (b & 0x40) != 0,
        seq: b & 0x3F,
    };
}

/// Encodes a 1-byte DNP3 Transport Layer header.
pub fn encodeDnp3Transport(fin: bool, fir: bool, seq: u8): u8 {
    let b: u8 = seq & 0x3F;
    if (fin) {
        b = b | 0x80;
    }
    if (fir) {
        b = b | 0x40;
    }
    return b;
}

/// Parses a DNP3 Application Layer fragment.
/// Request function codes (`< 129`) have a 2-byte header (`iin = null`).
/// Response function codes (`>= 129`, e.g. `FC_RESPONSE`, `FC_UNSOLICITED_RESPONSE`) have a 4-byte header with 2-byte LE IIN.
pub fn parseDnp3App(data: []const u8): Dnp3AppHeader? {
    if (data.len < 2) {
        return null;
    }
    const ac = data[0];
    const fc = data[1];
    const fir = (ac & 0x80) != 0;
    const fin = (ac & 0x40) != 0;
    const con = (ac & 0x20) != 0;
    const uns = (ac & 0x10) != 0;
    const seq = ac & 0x0F;

    if (fc >= FC_RESPONSE) {
        if (data.len < 4) {
            return null;
        }
        const iin = readU16Le(data, 2);
        return Dnp3AppHeader {
            fir: fir,
            fin: fin,
            con: con,
            uns: uns,
            seq: seq,
            func_code: fc,
            iin: iin,
            objects: data[4..],
        };
    }

    return Dnp3AppHeader {
        fir: fir,
        fin: fin,
        con: con,
        uns: uns,
        seq: seq,
        func_code: fc,
        iin: null,
        objects: data[2..],
    };
}
