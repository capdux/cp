// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2516 PPPoE Discovery & Session Codec)

pub const PPPOE_ETHERTYPE_DISCOVERY: u16 = 0x8863;
pub const PPPOE_ETHERTYPE_SESSION: u16 = 0x8864;
pub const PPPOE_VERSION: u8 = 1;
pub const PPPOE_TYPE: u8 = 1;
pub const PPPOE_HEADER_SIZE: usize = 6;
pub const PPPOE_TAG_HEADER_SIZE: usize = 4;

// PPPoE Codes (RFC 2516)
pub const CODE_SESSION: u8 = 0x00;
pub const CODE_PADO: u8 = 0x07;
pub const CODE_PADI: u8 = 0x09;
pub const CODE_PADR: u8 = 0x19;
pub const CODE_PADS: u8 = 0x65;
pub const CODE_PADT: u8 = 0xA7;

// PPPoE Discovery Tag Types (RFC 2516)
pub const TAG_END_OF_LIST: u16 = 0x0000;
pub const TAG_SERVICE_NAME: u16 = 0x0101;
pub const TAG_AC_NAME: u16 = 0x0102;
pub const TAG_HOST_UNIQ: u16 = 0x0103;
pub const TAG_AC_COOKIE: u16 = 0x0104;
pub const TAG_VENDOR_SPECIFIC: u16 = 0x0105;
pub const TAG_RELAY_SESSION_ID: u16 = 0x0110;
pub const TAG_SERVICE_NAME_ERROR: u16 = 0x0201;
pub const TAG_AC_SYSTEM_ERROR: u16 = 0x0202;
pub const TAG_GENERIC_ERROR: u16 = 0x0203;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn isValidDiscoveryCode(code: u8): bool {
    return code == CODE_PADO ||
        code == CODE_PADI ||
        code == CODE_PADR ||
        code == CODE_PADS ||
        code == CODE_PADT;
}

pub struct PppoeHeader {
    version: u8,
    pkt_type: u8,
    code: u8,
    session_id: u16,
    length: u16,
}

pub struct PppoeTag {
    tag_type: u16,
    tag_len: u16,
    value: []const u8,
}

pub struct PppoeTagIter {
    data: []const u8,
    offset: usize,
    done: bool,
    failed: bool,

    fn init(payload: []const u8): PppoeTagIter {
        return PppoeTagIter{
            .data = payload,
            .offset = 0,
            .done = false,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): PppoeTag? {
        if (_.done || _.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < PPPOE_TAG_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const tag_type = readU16Be(_.data, _.offset);
        const tag_len = readU16Be(_.data, _.offset + 2);
        const len_u = tag_len as usize;
        if (PPPOE_TAG_HEADER_SIZE + len_u > avail) {
            _.failed = true;
            return null;
        }
        const val_start = _.offset + PPPOE_TAG_HEADER_SIZE;
        const val_end = val_start + len_u;
        _.offset = val_end;

        if (tag_type == TAG_END_OF_LIST) {
            _.done = true;
            return null;
        }
        return PppoeTag{
            .tag_type = tag_type,
            .tag_len = tag_len,
            .value = _.data[val_start..val_end],
        };
    }
}

pub struct PppoePacket {
    header: PppoeHeader,
    payload: []const u8,

    fn isSession(): bool {
        return _.header.code == CODE_SESSION;
    }

    fn isDiscovery(): bool {
        return isValidDiscoveryCode(_.header.code);
    }

    fn tags(): PppoeTagIter {
        return PppoeTagIter.init(_.payload);
    }

    fn findTag(tag_type: u16): PppoeTag? {
        let it = _.tags();
        while (true) {
            if (tag := it.next()) {
                if (tag.tag_type == tag_type) {
                    return tag;
                }
            } else {
                break;
            }
        }
        return null;
    }

    fn serviceName(): ([]const u8)? {
        if (tag := _.findTag(TAG_SERVICE_NAME)) {
            return tag.value;
        }
        return null;
    }

    fn acName(): ([]const u8)? {
        if (tag := _.findTag(TAG_AC_NAME)) {
            return tag.value;
        }
        return null;
    }

    fn hostUniq(): ([]const u8)? {
        if (tag := _.findTag(TAG_HOST_UNIQ)) {
            return tag.value;
        }
        return null;
    }

    fn acCookie(): ([]const u8)? {
        if (tag := _.findTag(TAG_AC_COOKIE)) {
            return tag.value;
        }
        return null;
    }
}

/// Fast header check: verifies len >= 6, ver == 1, type == 1, and payload bounds 6 + length <= data.len.
pub fn isPppoe(data: []const u8): bool {
    if (data.len < PPPOE_HEADER_SIZE) {
        return false;
    }
    const ver = (data[0] >> 4) & 0x0F;
    const ptype = data[0] & 0x0F;
    if (ver != PPPOE_VERSION || ptype != PPPOE_TYPE) {
        return false;
    }
    const length = readU16Be(data, 4) as usize;
    return PPPOE_HEADER_SIZE + length <= data.len;
}

/// Parses an RFC 2516 PPPoE Discovery or Session packet with strict bounds and tag validation.
pub fn parsePppoe(data: []const u8): PppoePacket? {
    if (!isPppoe(data)) {
        return null;
    }
    const ver = (data[0] >> 4) & 0x0F;
    const ptype = data[0] & 0x0F;
    const code = data[1];
    const session_id = readU16Be(data, 2);
    const length = readU16Be(data, 4);
    const payload = data[PPPOE_HEADER_SIZE..(PPPOE_HEADER_SIZE + (length as usize))];

    if (code == CODE_SESSION) {
        if (session_id == 0) {
            return null;
        }
    } else if (isValidDiscoveryCode(code)) {
        let it = PppoeTagIter.init(payload);
        while (true) {
            if (tag := it.next()) {
                * = tag;
            } else {
                break;
            }
        }
        if (it.hasError()) {
            return null;
        }
    } else {
        return null;
    }

    const hdr = PppoeHeader{
        .version = ver,
        .pkt_type = ptype,
        .code = code,
        .session_id = session_id,
        .length = length,
    };
    return PppoePacket{
        .header = hdr,
        .payload = payload,
    };
}

/// Zero-alloc PPPoE packet encoder supporting both Discovery TLV tags and Session payloads.
pub struct PppoeWriter {
    buf: []u8,
    pos: usize,
    valid: bool,

    fn init(buf: []u8, code: u8, session_id: u16): PppoeWriter {
        if (buf.len < PPPOE_HEADER_SIZE) {
            return PppoeWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        if (code == CODE_SESSION && session_id == 0) {
            return PppoeWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        if (code != CODE_SESSION && !isValidDiscoveryCode(code)) {
            return PppoeWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        buf[0] = ((PPPOE_VERSION << 4) | PPPOE_TYPE) as u8;
        buf[1] = code;
        writeU16Be(buf, 2, session_id);
        writeU16Be(buf, 4, 0);
        return PppoeWriter{
            .buf = buf,
            .pos = PPPOE_HEADER_SIZE,
            .valid = true,
        };
    }

    fn remaining(): usize {
        if (!_.valid || _.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeTag(*_, tag_type: u16, value: []const u8): bool {
        if (!_.valid) {
            return false;
        }
        if (value.len > 65535) {
            return false;
        }
        const need = PPPOE_TAG_HEADER_SIZE + value.len;
        const new_payload_len = (_.pos - PPPOE_HEADER_SIZE) + need;
        if (new_payload_len > 65535 || _.remaining() < need) {
            return false;
        }
        writeU16Be(_.buf, _.pos, tag_type);
        writeU16Be(_.buf, _.pos + 2, value.len as u16);
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + PPPOE_TAG_HEADER_SIZE + i] = value[i];
            i += 1;
        }
        _.pos += need;
        return true;
    }

    fn writeEndOfList(*_): bool {
        return _.writeTag(TAG_END_OF_LIST, "");
    }

    fn writePayload(*_, payload: []const u8): bool {
        if (!_.valid) {
            return false;
        }
        const new_payload_len = (_.pos - PPPOE_HEADER_SIZE) + payload.len;
        if (new_payload_len > 65535 || _.remaining() < payload.len) {
            return false;
        }
        let i: usize = 0;
        while (i < payload.len) {
            _.buf[_.pos + i] = payload[i];
            i += 1;
        }
        _.pos += payload.len;
        return true;
    }

    fn finish(*_): ([]const u8)? {
        if (!_.valid || _.pos < PPPOE_HEADER_SIZE) {
            return null;
        }
        const payload_len = _.pos - PPPOE_HEADER_SIZE;
        if (payload_len > 65535) {
            return null;
        }
        writeU16Be(_.buf, 4, payload_len as u16);
        return _.buf[0.._.pos];
    }
}
