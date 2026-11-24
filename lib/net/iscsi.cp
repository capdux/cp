// Origin: Pure-CP Stdlib (Zero-Alloc RFC 7143 iSCSI BHS, KV & CRC32C Wire Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (RFC 7143 iSCSI)
// ---------------------------------------------------------------------------

pub const ISCSI_TCP_PORT: u16 = 3260;
pub const ISCSI_BHS_SIZE: usize = 48;
pub const ISCSI_DIGEST_SIZE: usize = 4;
pub const ISCSI_CRC32C_POLY: u32 = 0x82F63B78;

// iSCSI Opcodes (Low 6 bits of byte 0)
pub const OP_NOP_OUT: u8 = 0x00;
pub const OP_SCSI_CMD: u8 = 0x01;
pub const OP_TASK_MGMT_REQ: u8 = 0x02;
pub const OP_LOGIN_REQ: u8 = 0x03;
pub const OP_TEXT_REQ: u8 = 0x04;
pub const OP_DATA_OUT: u8 = 0x05;
pub const OP_LOGOUT_REQ: u8 = 0x06;
pub const OP_NOP_IN: u8 = 0x20;
pub const OP_SCSI_RESP: u8 = 0x21;
pub const OP_TASK_MGMT_RESP: u8 = 0x22;
pub const OP_LOGIN_RESP: u8 = 0x23;
pub const OP_TEXT_RESP: u8 = 0x24;
pub const OP_DATA_IN: u8 = 0x25;
pub const OP_LOGOUT_RESP: u8 = 0x26;
pub const OP_R2T: u8 = 0x31;
pub const OP_ASYNC_MSG: u8 = 0x32;
pub const OP_REJECT: u8 = 0x3F;

// ---------------------------------------------------------------------------
// Internal Byte Helpers
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU24Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    return (b0 << 16) | (b1 << 8) | b2;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn align4(len: usize): usize {
    const rem = len % 4;
    if (rem == 0) {
        return len;
    }
    return len + (4 - rem);
}

// ---------------------------------------------------------------------------
// Castagnoli CRC32C (RFC 7143 §12.1)
// ---------------------------------------------------------------------------

/// Computes Castagnoli CRC32C (reflected polynomial `0x82F63B78`, init `0xFFFFFFFF`, final `^ 0xFFFFFFFF`).
pub fn iscsiCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ ISCSI_CRC32C_POLY;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// BHS, PDU, SCSI Command, Login Request & KV Parameter Models
// ---------------------------------------------------------------------------

/// Decoded 48-byte iSCSI Basic Header Segment (BHS).
pub struct IscsiBhs {
    immediate: bool,
    opcode: u8,
    final_flag: bool,
    opcode_flags: u8,
    total_ahs_len_words: u8,
    data_segment_len: u32,
    lun: [8]u8,
    initiator_task_tag: u32,
    raw_bhs: []const u8,

    /// Total Additional Header Segment (AHS) length in bytes (`total_ahs_len_words * 4`).
    fn ahsBytesLen(): usize {
        return (_.total_ahs_len_words as usize) * 4;
    }

    /// 4-byte aligned Data Segment length on the wire.
    fn paddedDataLen(): usize {
        return align4(_.data_segment_len as usize);
    }
}

/// Zero-copy view of a parsed iSCSI PDU (BHS + AHS + DataSegment + optional CRC32C digests).
pub struct IscsiPdu {
    bhs: IscsiBhs,
    ahs: []const u8,
    data_segment: []const u8,
    consumed: usize,
}

/// Decoded iSCSI SCSI Command (`OP_SCSI_CMD`) BHS fields including 16-byte CDB.
pub struct IscsiScsiCmd {
    read_flag: bool,
    write_flag: bool,
    attr: u8,
    expected_data_len: u32,
    cmd_sn: u32,
    exp_stat_sn: u32,
    cdb: [16]u8,
}

/// Decoded iSCSI Login Request (`OP_LOGIN_REQ`) BHS fields.
pub struct IscsiLoginReq {
    transit: bool,
    cont: bool,
    csg: u8,
    nsg: u8,
    version_max: u8,
    version_min: u8,
    isid: [6]u8,
    tsih: u16,
    cid: u16,
    cmd_sn: u32,
    exp_stat_sn: u32,
}

/// A single zero-copy `Key=Value` parameter from an iSCSI Login/Text Data Segment.
pub struct IscsiKv {
    key: []const u8,
    value: []const u8,
}

/// Zero-allocation iterator over NUL-terminated `Key=Value\0` pairs in Login/Text Data Segments.
pub struct IscsiKvIter {
    data: []const u8,
    offset: usize,

    pub static fn init(data_seg: []const u8): IscsiKvIter {
        return IscsiKvIter {
            data: data_seg,
            offset: 0,
        };
    }

    fn next(*_): IscsiKv? {
        while (_.offset < _.data.len) {
            if (_.data[_.offset] == 0) {
                _.offset += 1;
                continue;
            }
            const start = _.offset;
            let eq_pos: usize? = null;
            while (_.offset < _.data.len && _.data[_.offset] != 0) {
                if (_.data[_.offset] == '=' && eq_pos == null) {
                    eq_pos = _.offset;
                }
                _.offset += 1;
            }
            const end = _.offset;
            if (_.offset < _.data.len && _.data[_.offset] == 0) {
                _.offset += 1;
            }
            if (eq := eq_pos) {
                const key = _.data[start..eq];
                if (key.len == 0) {
                    continue;
                }
                return IscsiKv {
                    key: key,
                    value: _.data[eq + 1..end],
                };
            }
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// PDU & Sub-Header Parsers
// ---------------------------------------------------------------------------

/// Parses a complete iSCSI PDU from `data`.
/// Validates 48B BHS, AHS, optional 4B LE HeaderDigest (CRC32C), 4B-aligned Data Segment,
/// and optional 4B LE DataDigest (CRC32C when `data_segment_len > 0`).
pub fn parseIscsiPdu(
    data: []const u8,
    has_header_digest: bool,
    has_data_digest: bool,
): IscsiPdu? {
    if (data.len < ISCSI_BHS_SIZE) {
        return null;
    }

    const b0 = data[0];
    const b1 = data[1];
    const immediate = (b0 & 0x40) != 0;
    const opcode = b0 & 0x3F;
    const final_flag = (b1 & 0x80) != 0;
    const opcode_flags = b1 & 0x7F;
    const total_ahs_len_words = data[4];
    const data_segment_len = readU24Be(data, 5);

    let lun: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) {
        lun[i] = data[8 + i];
        i += 1;
    }
    const itt = readU32Be(data, 16);

    const bhs = IscsiBhs {
        immediate: immediate,
        opcode: opcode,
        final_flag: final_flag,
        opcode_flags: opcode_flags,
        total_ahs_len_words: total_ahs_len_words,
        data_segment_len: data_segment_len,
        lun: lun,
        initiator_task_tag: itt,
        raw_bhs: data[0..ISCSI_BHS_SIZE],
    };

    const ahs_len = bhs.ahsBytesLen();
    const hdr_total = ISCSI_BHS_SIZE + ahs_len;
    if (hdr_total > data.len) {
        return null;
    }
    const ahs = data[ISCSI_BHS_SIZE..hdr_total];

    let pos = hdr_total;
    if (has_header_digest) {
        if (data.len - pos < ISCSI_DIGEST_SIZE) {
            return null;
        }
        const expected = iscsiCrc32c(data[0..hdr_total]);
        const wire_crc = readU32Le(data, pos);
        if (wire_crc != expected) {
            return null;
        }
        pos += ISCSI_DIGEST_SIZE;
    }

    const dlen = data_segment_len as usize;
    const padded_len = bhs.paddedDataLen();
    if (padded_len > data.len - pos) {
        return null;
    }
    const data_start = pos;
    const data_segment = data[data_start..data_start + dlen];
    pos += padded_len;

    if (has_data_digest && dlen > 0) {
        if (data.len - pos < ISCSI_DIGEST_SIZE) {
            return null;
        }
        const expected = iscsiCrc32c(data[data_start..data_start + padded_len]);
        const wire_crc = readU32Le(data, pos);
        if (wire_crc != expected) {
            return null;
        }
        pos += ISCSI_DIGEST_SIZE;
    }

    return IscsiPdu {
        bhs: bhs,
        ahs: ahs,
        data_segment: data_segment,
        consumed: pos,
    };
}

/// Decodes SCSI Command BHS fields from an `OP_SCSI_CMD` PDU.
pub fn parseIscsiScsiCmd(pdu: IscsiPdu): IscsiScsiCmd? {
    if (pdu.bhs.opcode != OP_SCSI_CMD || pdu.bhs.raw_bhs.len < ISCSI_BHS_SIZE) {
        return null;
    }
    const b = pdu.bhs.raw_bhs;
    let cdb: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        cdb[i] = b[32 + i];
        i += 1;
    }
    return IscsiScsiCmd {
        read_flag: (b[1] & 0x40) != 0,
        write_flag: (b[1] & 0x20) != 0,
        attr: b[1] & 0x07,
        expected_data_len: readU32Be(b, 20),
        cmd_sn: readU32Be(b, 24),
        exp_stat_sn: readU32Be(b, 28),
        cdb: cdb,
    };
}

/// Decodes Login Request BHS fields from an `OP_LOGIN_REQ` PDU.
pub fn parseIscsiLoginReq(pdu: IscsiPdu): IscsiLoginReq? {
    if (pdu.bhs.opcode != OP_LOGIN_REQ || pdu.bhs.raw_bhs.len < ISCSI_BHS_SIZE) {
        return null;
    }
    const b = pdu.bhs.raw_bhs;
    let isid: [6]u8 = [0; 6];
    let i: usize = 0;
    while (i < 6) {
        isid[i] = b[8 + i];
        i += 1;
    }
    return IscsiLoginReq {
        transit: (b[1] & 0x80) != 0,
        cont: (b[1] & 0x40) != 0,
        csg: (b[1] >> 2) & 0x03,
        nsg: b[1] & 0x03,
        version_max: b[2],
        version_min: b[3],
        isid: isid,
        tsih: readU16Be(b, 14),
        cid: readU16Be(b, 20),
        cmd_sn: readU32Be(b, 24),
        exp_stat_sn: readU32Be(b, 28),
    };
}

/// Looks up the first `Key=Value` parameter matching `key` in an iSCSI Login/Text Data Segment.
pub fn findIscsiKv(data_seg: []const u8, key: []const u8): ([]const u8)? {
    let it = IscsiKvIter.init(data_seg);
    while (kv := it.next()) {
        if (bytesEq(kv.key, key)) {
            return kv.value;
        }
    }
    return null;
}
