// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1112/2236/3376 IGMP v1/v2/v3 Codec) | Created: 2026-11-22
// Pure CP zero-heap-allocation IGMP v1/v2/v3 multicast membership protocol codec:
// RFC 1071 16-bit checksum, RFC 3376 floating-point time code decoder, v1/v2/v3 Query &
// v3 Membership Report parser, zero-alloc Group Record iterator, and v2 8-byte encoder.

/// IP protocol number for IGMP (`2`).
pub const IGMP_IP_PROTO: u8 = 2;

/// Minimum IGMP packet size in bytes (`8`, RFC 1112 / RFC 2236).
pub const IGMP_MIN_SIZE: usize = 8;

/// Minimum IGMPv3 Membership Query size in bytes (`12`, RFC 3376).
pub const IGMP_V3_QUERY_MIN_SIZE: usize = 12;

/// IGMP message types (RFC 1112 / RFC 2236 / RFC 3376).
pub enum IgmpType: u8 {
    unknown = 0,
    membership_query = 0x11,
    v1_report = 0x12,
    v2_report = 0x16,
    v2_leave = 0x17,
    v3_report = 0x22,
}

/// IGMPv3 Group Record types (RFC 3376 Section 4.2.12).
pub enum IgmpV3RecordType: u8 {
    unknown = 0,
    mode_is_include = 1,
    mode_is_exclude = 2,
    change_to_include = 3,
    change_to_exclude = 4,
    allow_new_sources = 5,
    block_old_sources = 6,
}

/// Zero-copy decoded IGMPv3 Group Record entry inside a v3 Membership Report.
pub struct IgmpV3GroupRecord {
    rec_type: IgmpV3RecordType = IgmpV3RecordType.unknown,
    raw_rec_type: u8 = 0,
    aux_data_words: u8 = 0,
    num_sources: u16 = 0,
    multicast_addr: [4]u8 = [0; 4],
    sources_slice: []const u8 = "",
    aux_slice: []const u8 = "",

    /// Returns the `idx`-th IPv4 source address (`0 <= idx < num_sources`), or `null` if out of range.
    fn sourceAt(idx: usize): ([4]u8)? {
        if (idx >= (_.num_sources as usize)) {
            return null;
        }
        const off = idx * 4;
        if (off + 4 > _.sources_slice.len) {
            return null;
        }
        return [
            _.sources_slice[off],
            _.sources_slice[off + 1],
            _.sources_slice[off + 2],
            _.sources_slice[off + 3],
        ];
    }
}

/// Zero-copy decoded IGMP v1/v2/v3 packet.
pub struct IgmpPacket {
    msg_type: IgmpType = IgmpType.unknown,
    raw_type: u8 = 0,
    max_resp_code: u8 = 0,
    checksum: u16 = 0,
    group_addr: [4]u8 = [0; 4],
    // IGMPv3 Query fields (valid when is_v3_query == true)
    is_v3_query: bool = false,
    suppress_router_side: bool = false,
    qrv: u8 = 0,
    qqic: u8 = 0,
    num_sources: u16 = 0,
    sources_slice: []const u8 = "",
    // IGMPv3 Membership Report fields (valid when msg_type == IgmpType.v3_report)
    num_group_records: u16 = 0,
    records_slice: []const u8 = "",

    /// Returns the `idx`-th IPv4 source address of an IGMPv3 Membership Query, or `null`.
    fn querySourceAt(idx: usize): ([4]u8)? {
        if (!_.is_v3_query || idx >= (_.num_sources as usize)) {
            return null;
        }
        const off = idx * 4;
        if (off + 4 > _.sources_slice.len) {
            return null;
        }
        return [
            _.sources_slice[off],
            _.sources_slice[off + 1],
            _.sources_slice[off + 2],
            _.sources_slice[off + 3],
        ];
    }

    /// Returns a zero-allocation iterator over IGMPv3 Group Records in a v3 Membership Report.
    fn v3Records(): IgmpV3RecordIter {
        return IgmpV3RecordIter.init(_.records_slice, _.num_group_records);
    }
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn writeBeU16(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn classifyIgmpType(raw: u8): IgmpType {
    if (raw == 0x11) { return IgmpType.membership_query; }
    if (raw == 0x12) { return IgmpType.v1_report; }
    if (raw == 0x16) { return IgmpType.v2_report; }
    if (raw == 0x17) { return IgmpType.v2_leave; }
    if (raw == 0x22) { return IgmpType.v3_report; }
    return IgmpType.unknown;
}

fn classifyV3RecordType(raw: u8): IgmpV3RecordType {
    if (raw == 1) { return IgmpV3RecordType.mode_is_include; }
    if (raw == 2) { return IgmpV3RecordType.mode_is_exclude; }
    if (raw == 3) { return IgmpV3RecordType.change_to_include; }
    if (raw == 4) { return IgmpV3RecordType.change_to_exclude; }
    if (raw == 5) { return IgmpV3RecordType.allow_new_sources; }
    if (raw == 6) { return IgmpV3RecordType.block_old_sources; }
    return IgmpV3RecordType.unknown;
}

fn igmpTypeToByte(t: IgmpType): u8 {
    if (t == IgmpType.membership_query) { return 0x11; }
    if (t == IgmpType.v1_report) { return 0x12; }
    if (t == IgmpType.v2_report) { return 0x16; }
    if (t == IgmpType.v2_leave) { return 0x17; }
    if (t == IgmpType.v3_report) { return 0x22; }
    return 0;
}

// ---------------------------------------------------------------------------
// RFC 1071 Checksum & RFC 3376 Float Time Code Decoder
// ---------------------------------------------------------------------------

/// Computes the RFC 1071 16-bit one's complement checksum over `data`.
pub fn igmpChecksum(data: []const u8): u16 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i + 1 < data.len) {
        sum += (((data[i] as u32) << 8) | (data[i + 1] as u32));
        i += 2;
    }
    if (i < data.len) {
        sum += (data[i] as u32) << 8;
    }
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ((~sum) & 0xFFFF) as u16;
}

/// Verifies the RFC 1071 16-bit checksum of an IGMP packet (`>= 8` bytes).
pub fn verifyIgmpChecksum(packet: []const u8): bool {
    if (packet.len < IGMP_MIN_SIZE) {
        return false;
    }
    return igmpChecksum(packet) == 0;
}

/// Decodes RFC 3376 8-bit floating-point time codes (`Max Resp Code` / `QQIC`):
/// - `code < 128` -> `code`
/// - `code >= 128` -> `(mant | 0x10) << (exp + 3)`, where `exp = (code >> 4) & 0x07`, `mant = code & 0x0F`.
pub fn igmpDecodeFloatCode(code: u8): u32 {
    if (code < 128) {
        return code as u32;
    }
    const exp: u8 = (code >> 4) & 0x07;
    const mant: u32 = (code & 0x0F) as u32;
    const shift: u5 = (exp + 3) as u5;
    return (mant | 0x10) << shift;
}

// ---------------------------------------------------------------------------
// Public IGMP Parser & v3 Group Record Iterator
// ---------------------------------------------------------------------------

/// Parses an IGMP v1, v2, or v3 packet from `data`.
pub fn parseIgmp(data: []const u8): IgmpPacket? {
    if (data.len < IGMP_MIN_SIZE) {
        return null;
    }
    const raw_type = data[0];
    const msg_type = classifyIgmpType(raw_type);
    const max_resp_code = data[1];
    const checksum = readBeU16(data, 2);

    if (msg_type == IgmpType.v3_report) {
        const num_records = readBeU16(data, 6);
        return IgmpPacket {
            msg_type: msg_type,
            raw_type: raw_type,
            max_resp_code: max_resp_code,
            checksum: checksum,
            group_addr: [0, 0, 0, 0],
            is_v3_query: false,
            suppress_router_side: false,
            qrv: 0,
            qqic: 0,
            num_sources: 0,
            sources_slice: "",
            num_group_records: num_records,
            records_slice: data[8..data.len],
        };
    }

    const group_addr: [4]u8 = [data[4], data[5], data[6], data[7]];

    if (msg_type == IgmpType.membership_query && data.len > IGMP_MIN_SIZE) {
        if (data.len < IGMP_V3_QUERY_MIN_SIZE) {
            return null;
        }
        const s_qrv = data[8];
        const suppress = (s_qrv & 0x08) != 0;
        const qrv = s_qrv & 0x07;
        const qqic = data[9];
        const num_sources = readBeU16(data, 10);
        const sources_bytes = (num_sources as usize) * 4;
        if (data.len - IGMP_V3_QUERY_MIN_SIZE < sources_bytes) {
            return null;
        }
        return IgmpPacket {
            msg_type: msg_type,
            raw_type: raw_type,
            max_resp_code: max_resp_code,
            checksum: checksum,
            group_addr: group_addr,
            is_v3_query: true,
            suppress_router_side: suppress,
            qrv: qrv,
            qqic: qqic,
            num_sources: num_sources,
            sources_slice: data[IGMP_V3_QUERY_MIN_SIZE..(IGMP_V3_QUERY_MIN_SIZE + sources_bytes)],
            num_group_records: 0,
            records_slice: "",
        };
    }

    return IgmpPacket {
        msg_type: msg_type,
        raw_type: raw_type,
        max_resp_code: max_resp_code,
        checksum: checksum,
        group_addr: group_addr,
        is_v3_query: false,
        suppress_router_side: false,
        qrv: 0,
        qqic: 0,
        num_sources: 0,
        sources_slice: "",
        num_group_records: 0,
        records_slice: "",
    };
}

/// Zero-allocation iterator over RFC 3376 IGMPv3 Group Records.
pub struct IgmpV3RecordIter {
    data: []const u8 = "",
    pos: usize = 0,
    remaining: u16 = 0,
    failed: bool = false,

    pub static fn init(records_slice: []const u8, num_records: u16): IgmpV3RecordIter {
        return IgmpV3RecordIter {
            data: records_slice,
            pos: 0,
            remaining: num_records,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): IgmpV3GroupRecord? {
        if (_.failed || _.remaining == 0) {
            return null;
        }
        if (_.pos >= _.data.len || _.data.len - _.pos < 8) {
            _.failed = true;
            return null;
        }
        const raw_rec = _.data[_.pos];
        const aux_words = _.data[_.pos + 1];
        const num_sources = readBeU16(_.data, _.pos + 2);
        const mcast: [4]u8 = [
            _.data[_.pos + 4],
            _.data[_.pos + 5],
            _.data[_.pos + 6],
            _.data[_.pos + 7],
        ];
        const sources_bytes = (num_sources as usize) * 4;
        const aux_bytes = (aux_words as usize) * 4;
        const body_bytes = sources_bytes + aux_bytes;
        if (_.data.len - (_.pos + 8) < body_bytes) {
            _.failed = true;
            return null;
        }
        const src_start = _.pos + 8;
        const src_end = src_start + sources_bytes;
        const aux_end = src_end + aux_bytes;
        _.pos = aux_end;
        _.remaining -= 1;

        return IgmpV3GroupRecord {
            rec_type: classifyV3RecordType(raw_rec),
            raw_rec_type: raw_rec,
            aux_data_words: aux_words,
            num_sources: num_sources,
            multicast_addr: mcast,
            sources_slice: _.data[src_start..src_end],
            aux_slice: _.data[src_end..aux_end],
        };
    }
}

// ---------------------------------------------------------------------------
// Zero-Alloc IGMPv2 Encoder
// ---------------------------------------------------------------------------

/// Encodes an 8-byte IGMP v1/v2 packet (`membership_query`, `v1_report`, `v2_report`, `v2_leave`)
/// into `out` and automatically backpatches the RFC 1071 16-bit checksum.
pub fn encodeIgmpV2(
    msg_type: IgmpType,
    max_resp_code: u8,
    group_addr: [4]u8,
    out: []u8,
): usize? {
    if (out.len < IGMP_MIN_SIZE || msg_type == IgmpType.unknown || msg_type == IgmpType.v3_report) {
        return null;
    }
    out[0] = igmpTypeToByte(msg_type);
    out[1] = max_resp_code;
    out[2] = 0;
    out[3] = 0;
    out[4] = group_addr[0];
    out[5] = group_addr[1];
    out[6] = group_addr[2];
    out[7] = group_addr[3];

    const csum = igmpChecksum(out[0..IGMP_MIN_SIZE]);
    writeBeU16(out, 2, csum);
    return IGMP_MIN_SIZE;
}
