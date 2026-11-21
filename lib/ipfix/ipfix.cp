// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 7011 IPFIX v10 Wire Parser)

pub const IPFIX_PORT: u16 = 4739;
pub const IPFIX_TLS_PORT: u16 = 4740;
pub const IPFIX_VERSION: u16 = 10;
pub const IPFIX_HEADER_SIZE: usize = 16;
pub const IPFIX_SET_HEADER_SIZE: usize = 4;

pub const SET_ID_TEMPLATE: u16 = 2;
pub const SET_ID_OPTIONS_TEMPLATE: u16 = 3;
pub const SET_ID_DATA_MIN: u16 = 256;

pub const IE_ENTERPRISE_BIT: u16 = 0x8000;
pub const IE_VARLEN_INDICATOR: u16 = 0xFFFF;

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

fn allZero(buf: []const u8): bool {
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct IpfixHeader {
    version: u16,
    length: u16,
    export_time: u32,
    sequence_number: u32,
    observation_domain_id: u32,
}

pub struct IpfixFieldSpec {
    ie_id: u16,
    field_length: u16,
    enterprise_number: u32?,

    fn isEnterprise(): bool {
        return _.enterprise_number != null;
    }

    fn isVariableLength(): bool {
        return _.field_length == IE_VARLEN_INDICATOR;
    }
}

pub struct IpfixFieldSpecIter {
    data: []const u8,
    offset: usize,
    remaining_fields: u16,
    failed: bool,

    fn init(fields_bytes: []const u8, field_count: u16): IpfixFieldSpecIter {
        return IpfixFieldSpecIter{
            .data = fields_bytes,
            .offset = 0,
            .remaining_fields = field_count,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): IpfixFieldSpec? {
        if (_.failed || _.remaining_fields == 0) {
            return null;
        }
        if (_.offset + 4 > _.data.len) {
            _.failed = true;
            return null;
        }
        const raw_id = readU16Be(_.data, _.offset);
        const field_length = readU16Be(_.data, _.offset + 2);
        const is_ent = (raw_id & IE_ENTERPRISE_BIT) != 0;
        const ie_id = raw_id & 0x7FFF;

        let ent_num: u32? = null;
        if (is_ent) {
            if (_.offset + 8 > _.data.len) {
                _.failed = true;
                return null;
            }
            ent_num = readU32Be(_.data, _.offset + 4);
            _.offset += 8;
        } else {
            _.offset += 4;
        }
        _.remaining_fields -= 1;

        return IpfixFieldSpec{
            .ie_id = ie_id,
            .field_length = field_length,
            .enterprise_number = ent_num,
        };
    }
}

pub struct IpfixTemplateRecord {
    template_id: u16,
    field_count: u16,
    fields_bytes: []const u8,

    fn fields(): IpfixFieldSpecIter {
        return IpfixFieldSpecIter.init(_.fields_bytes, _.field_count);
    }
}

/// Zero-copy iterator over Template Records inside a Template Set (Set ID 2),
/// handling both standard 4B and Enterprise 8B field specifiers and trailing zero padding.
pub struct IpfixTemplateIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    fn init(set_body: []const u8): IpfixTemplateIter {
        return IpfixTemplateIter{
            .data = set_body,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): IpfixTemplateRecord? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < 4) {
            // RFC 7011 Section 3.3.1: trailing padding (< 4B) must be zero bytes
            if (allZero(_.data[_.offset..])) {
                _.offset = _.data.len;
                return null;
            }
            _.failed = true;
            return null;
        }

        const template_id = readU16Be(_.data, _.offset);
        const field_count = readU16Be(_.data, _.offset + 2);
        if (template_id == 0 && field_count == 0 && allZero(_.data[_.offset..])) {
            _.offset = _.data.len;
            return null;
        }

        const fields_start = _.offset + 4;
        let cur = fields_start;
        let idx: u16 = 0;
        while (idx < field_count) {
            if (cur + 4 > _.data.len) {
                _.failed = true;
                return null;
            }
            const raw_id = readU16Be(_.data, cur);
            const is_ent = (raw_id & IE_ENTERPRISE_BIT) != 0;
            if (is_ent) {
                if (cur + 8 > _.data.len) {
                    _.failed = true;
                    return null;
                }
                cur += 8;
            } else {
                cur += 4;
            }
            idx += 1;
        }

        _.offset = cur;
        return IpfixTemplateRecord{
            .template_id = template_id,
            .field_count = field_count,
            .fields_bytes = _.data[fields_start..cur],
        };
    }
}

pub struct IpfixSet {
    set_id: u16,
    length: u16,
    body: []const u8,

    fn isTemplate(): bool {
        return _.set_id == SET_ID_TEMPLATE;
    }

    fn isOptionsTemplate(): bool {
        return _.set_id == SET_ID_OPTIONS_TEMPLATE;
    }

    fn isData(): bool {
        return _.set_id >= SET_ID_DATA_MIN;
    }

    fn templates(): IpfixTemplateIter {
        return IpfixTemplateIter.init(_.body);
    }
}

pub struct IpfixSetIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    fn init(sets_bytes: []const u8): IpfixSetIter {
        return IpfixSetIter{
            .data = sets_bytes,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): IpfixSet? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < IPFIX_SET_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const set_id = readU16Be(_.data, _.offset);
        const length = readU16Be(_.data, _.offset + 2);
        const len_u = length as usize;
        if (len_u < IPFIX_SET_HEADER_SIZE || len_u > avail) {
            _.failed = true;
            return null;
        }
        const body_start = _.offset + IPFIX_SET_HEADER_SIZE;
        const body_end = _.offset + len_u;
        _.offset = body_end;
        return IpfixSet{
            .set_id = set_id,
            .length = length,
            .body = _.data[body_start..body_end],
        };
    }
}

pub struct IpfixMessage {
    header: IpfixHeader,
    sets_bytes: []const u8,

    fn sets(): IpfixSetIter {
        return IpfixSetIter.init(_.sets_bytes);
    }
}

/// Fast header check: verifies len >= 16, version == 10, and 16 <= length <= data.len.
pub fn isIpfix(data: []const u8): bool {
    if (data.len < IPFIX_HEADER_SIZE) {
        return false;
    }
    if (readU16Be(data, 0) != IPFIX_VERSION) {
        return false;
    }
    const msg_len = readU16Be(data, 2) as usize;
    return msg_len >= IPFIX_HEADER_SIZE && msg_len <= data.len;
}

/// Parses an RFC 7011 IPFIX v10 message and strictly validates all Set header bounds.
pub fn parseIpfix(data: []const u8): IpfixMessage? {
    if (!isIpfix(data)) {
        return null;
    }
    const msg_len = readU16Be(data, 2);
    const sets_slice = data[IPFIX_HEADER_SIZE..(msg_len as usize)];

    // Pre-validate all Set boundaries
    let it = IpfixSetIter.init(sets_slice);
    while (true) {
        if (s := it.next()) {
            * = s;
        } else {
            break;
        }
    }
    if (it.hasError()) {
        return null;
    }

    const hdr = IpfixHeader{
        .version = IPFIX_VERSION,
        .length = msg_len,
        .export_time = readU32Be(data, 4),
        .sequence_number = readU32Be(data, 8),
        .observation_domain_id = readU32Be(data, 12),
    };
    return IpfixMessage{
        .header = hdr,
        .sets_bytes = sets_slice,
    };
}

/// Decoded RFC 7011 Section 7 Variable-Length Information Element value.
pub struct IpfixVarLenValue {
    value: []const u8,
    consumed: usize,
}

/// Decodes an RFC 7011 Section 7 Variable-Length field:
/// - First byte < 255: 1-byte length prefix
/// - First byte == 255: followed by 2-byte Big-Endian length prefix
pub fn decodeIpfixVarLen(data: []const u8): IpfixVarLenValue? {
    if (data.len < 1) {
        return null;
    }
    const first = data[0];
    if (first < 255) {
        const len_u = first as usize;
        if (1 + len_u > data.len) {
            return null;
        }
        return IpfixVarLenValue{
            .value = data[1..(1 + len_u)],
            .consumed = 1 + len_u,
        };
    }
    if (data.len < 3) {
        return null;
    }
    const ext_len = readU16Be(data, 1) as usize;
    if (3 + ext_len > data.len) {
        return null;
    }
    return IpfixVarLenValue{
        .value = data[3..(3 + ext_len)],
        .consumed = 3 + ext_len,
    };
}
