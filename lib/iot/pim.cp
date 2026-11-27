// Origin: Pure-CP Stdlib (Zero-Alloc RFC 7761 PIM-SM v2 Multicast Wire Codec) | Created: 2026-11-22

pub const PIM_IP_PROTO: u8 = 103;
pub const PIM_VERSION: u8 = 2;
pub const PIM_HEADER_SIZE: usize = 4;
pub const PIM_HELLO_OPT_HEADER_SIZE: usize = 4;

// PIM v2 Message Types (RFC 7761)
pub const PIM_HELLO: u8 = 0;
pub const PIM_REGISTER: u8 = 1;
pub const PIM_REGISTER_STOP: u8 = 2;
pub const PIM_JOIN_PRUNE: u8 = 3;
pub const PIM_BOOTSTRAP: u8 = 4;
pub const PIM_ASSERT: u8 = 5;
pub const PIM_GRAFT: u8 = 6;
pub const PIM_GRAFT_ACK: u8 = 7;
pub const PIM_CANDIDATE_RP_ADV: u8 = 8;

// PIM Hello Option Types (RFC 7761 Section 4.9.2)
pub const OPT_HOLDTIME: u16 = 1;
pub const OPT_LAN_PRUNE_DELAY: u16 = 2;
pub const OPT_DR_PRIORITY: u16 = 19;
pub const OPT_GENERATION_ID: u16 = 20;
pub const OPT_ADDRESS_LIST: u16 = 24;

// PIM Register Flags (RFC 7761 Section 4.9.3)
pub const REG_FLAG_BORDER: u32 = 0x8000_0000;
pub const REG_FLAG_NULL: u32 = 0x4000_0000;

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

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn checksumEffectiveLen(data: []const u8): usize {
    // RFC 7761 Section 4.9: for Register messages, checksum covers only the first 8 bytes (4B hdr + 4B flags).
    if (data.len >= 8 && (data[0] & 0x0F) == PIM_REGISTER) {
        return 8;
    }
    return data.len;
}

fn sumSliceSkipCsum(init_sum: u32, data: []const u8, eff_len: usize): u32 {
    let sum = init_sum;
    let i: usize = 0;
    while (i + 1 < eff_len) {
        let hi: u32 = data[i] as u32;
        let lo: u32 = data[i + 1] as u32;
        if (i == 2) {
            hi = 0;
            lo = 0;
        }
        sum += (hi << 8) | lo;
        i += 2;
    }
    if (i < eff_len) {
        let last: u32 = data[i] as u32;
        if (i == 2) {
            last = 0;
        }
        sum += last << 8;
    }
    return sum;
}

fn finalizeOnesComplement(sum_in: u32): u16 {
    let sum = sum_in;
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ((sum ^ 0xFFFF) & 0xFFFF) as u16;
}

/// Computes RFC 7761 PIM checksum over IPv4 (no pseudo-header; Register messages cover first 8 bytes only).
pub fn pimChecksumIpv4(data: []const u8): u16 {
    const eff_len = checksumEffectiveLen(data);
    const sum = sumSliceSkipCsum(0, data, eff_len);
    return finalizeOnesComplement(sum);
}

/// Verifies PIM checksum over IPv4.
pub fn verifyPimChecksumIpv4(data: []const u8): bool {
    if (data.len < PIM_HEADER_SIZE) {
        return false;
    }
    return readU16Be(data, 2) == pimChecksumIpv4(data);
}

/// Computes RFC 7761 PIM checksum over IPv6 (includes 40-byte IPv6 pseudo-header; Register covers first 8 bytes).
pub fn pimChecksumIpv6(src_ip: [16]u8, dst_ip: [16]u8, data: []const u8): u16 {
    const eff_len = checksumEffectiveLen(data);
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < 16) {
        sum += ((src_ip[i] as u32) << 8) | (src_ip[i + 1] as u32);
        sum += ((dst_ip[i] as u32) << 8) | (dst_ip[i + 1] as u32);
        i += 2;
    }
    // Upper-Layer Packet Length (32-bit BE) + Next Header (103)
    const len32 = eff_len as u32;
    sum += (len32 >> 16) & 0xFFFF;
    sum += len32 & 0xFFFF;
    sum += PIM_IP_PROTO as u32;

    sum = sumSliceSkipCsum(sum, data, eff_len);
    return finalizeOnesComplement(sum);
}

/// Verifies PIM checksum over IPv6.
pub fn verifyPimChecksumIpv6(src_ip: [16]u8, dst_ip: [16]u8, data: []const u8): bool {
    if (data.len < PIM_HEADER_SIZE) {
        return false;
    }
    return readU16Be(data, 2) == pimChecksumIpv6(src_ip, dst_ip, data);
}

pub struct PimHeader {
    version: u8,
    msg_type: u8,
    reserved: u8,
    checksum: u16,
}

pub struct PimHelloOption {
    opt_type: u16,
    opt_len: u16,
    value: []const u8,
}

pub struct PimHelloOptIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    static fn init(body: []const u8): PimHelloOptIter {
        return PimHelloOptIter{
            .data = body,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): PimHelloOption? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < PIM_HELLO_OPT_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const opt_type = readU16Be(_.data, _.offset);
        const opt_len = readU16Be(_.data, _.offset + 2);
        const len_u = opt_len as usize;
        if (PIM_HELLO_OPT_HEADER_SIZE + len_u > avail) {
            _.failed = true;
            return null;
        }
        const val_start = _.offset + PIM_HELLO_OPT_HEADER_SIZE;
        const val_end = val_start + len_u;
        _.offset = val_end;
        return PimHelloOption{
            .opt_type = opt_type,
            .opt_len = opt_len,
            .value = _.data[val_start..val_end],
        };
    }
}

pub struct PimPacket {
    header: PimHeader,
    body: []const u8,

    fn helloOptions(): PimHelloOptIter {
        return PimHelloOptIter.init(_.body);
    }

    fn findHelloOption(opt_type: u16): PimHelloOption? {
        let it = _.helloOptions();
        while (true) {
            if (opt := it.next()) {
                if (opt.opt_type == opt_type) {
                    return opt;
                }
            } else {
                break;
            }
        }
        return null;
    }

    fn holdtime(): u16? {
        if (opt := _.findHelloOption(OPT_HOLDTIME)) {
            if (opt.value.len == 2) {
                return readU16Be(opt.value, 0);
            }
        }
        return null;
    }

    fn drPriority(): u32? {
        if (opt := _.findHelloOption(OPT_DR_PRIORITY)) {
            if (opt.value.len == 4) {
                return readU32Be(opt.value, 0);
            }
        }
        return null;
    }

    fn generationId(): u32? {
        if (opt := _.findHelloOption(OPT_GENERATION_ID)) {
            if (opt.value.len == 4) {
                return readU32Be(opt.value, 0);
            }
        }
        return null;
    }
}

pub struct PimRegister {
    flags: u32,
    inner_packet: []const u8,

    fn isBorder(): bool {
        return (_.flags & REG_FLAG_BORDER) != 0;
    }

    fn isNullRegister(): bool {
        return (_.flags & REG_FLAG_NULL) != 0;
    }
}

/// Parses a PIM Register body (4B flags + encapsulated inner IP packet).
pub fn parsePimRegister(body: []const u8): PimRegister? {
    if (body.len < 4) {
        return null;
    }
    return PimRegister{
        .flags = readU32Be(body, 0),
        .inner_packet = body[4..],
    };
}

/// Fast check verifying len >= 4, version == 2, and msg_type <= 8.
pub fn isPim(data: []const u8): bool {
    if (data.len < PIM_HEADER_SIZE) {
        return false;
    }
    const ver = (data[0] >> 4) & 0x0F;
    const mtype = data[0] & 0x0F;
    return ver == PIM_VERSION && mtype <= PIM_CANDIDATE_RP_ADV;
}

/// Parses a PIM-SM v2 packet and validates Hello TLV / Register body bounds.
pub fn parsePim(data: []const u8): PimPacket? {
    if (!isPim(data)) {
        return null;
    }
    const ver = (data[0] >> 4) & 0x0F;
    const mtype = data[0] & 0x0F;
    const body = data[PIM_HEADER_SIZE..];

    if (mtype == PIM_HELLO) {
        let it = PimHelloOptIter.init(body);
        while (true) {
            if (opt := it.next()) {
                * = opt;
            } else {
                break;
            }
        }
        if (it.hasError()) {
            return null;
        }
    } else if (mtype == PIM_REGISTER) {
        if (body.len < 4) {
            return null;
        }
    }

    const hdr = PimHeader{
        .version = ver,
        .msg_type = mtype,
        .reserved = data[1],
        .checksum = readU16Be(data, 2),
    };
    return PimPacket{
        .header = hdr,
        .body = body,
    };
}

/// Zero-alloc PIM v2 packet encoder with automatic IPv4 / IPv6 RFC 7761 checksum generation.
pub struct PimWriter {
    buf: []u8,
    pos: usize,
    valid: bool,

    static fn init(buf: []u8, msg_type: u8): PimWriter {
        if (buf.len < PIM_HEADER_SIZE || msg_type > PIM_CANDIDATE_RP_ADV) {
            return PimWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        buf[0] = ((PIM_VERSION << 4) | (msg_type & 0x0F)) as u8;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
        return PimWriter{
            .buf = buf,
            .pos = PIM_HEADER_SIZE,
            .valid = true,
        };
    }

    fn remaining(): usize {
        if (!_.valid || _.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeHelloOption(*_, opt_type: u16, value: []const u8): bool {
        if (!_.valid || value.len > 65535) {
            return false;
        }
        const need = PIM_HELLO_OPT_HEADER_SIZE + value.len;
        if (_.remaining() < need) {
            return false;
        }
        writeU16Be(_.buf, _.pos, opt_type);
        writeU16Be(_.buf, _.pos + 2, value.len as u16);
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + PIM_HELLO_OPT_HEADER_SIZE + i] = value[i];
            i += 1;
        }
        _.pos += need;
        return true;
    }

    fn writeHoldtime(*_, sec: u16): bool {
        let raw: [2]u8 = [0, 0];
        writeU16Be(raw[0..], 0, sec);
        return _.writeHelloOption(OPT_HOLDTIME, raw[0..]);
    }

    fn writeDrPriority(*_, priority: u32): bool {
        let raw: [4]u8 = [0, 0, 0, 0];
        writeU32Be(raw[0..], 0, priority);
        return _.writeHelloOption(OPT_DR_PRIORITY, raw[0..]);
    }

    fn writeGenerationId(*_, gen_id: u32): bool {
        let raw: [4]u8 = [0, 0, 0, 0];
        writeU32Be(raw[0..], 0, gen_id);
        return _.writeHelloOption(OPT_GENERATION_ID, raw[0..]);
    }

    fn writeRegister(*_, flags: u32, inner_packet: []const u8): bool {
        if (!_.valid) {
            return false;
        }
        const need = 4 + inner_packet.len;
        if (_.remaining() < need) {
            return false;
        }
        writeU32Be(_.buf, _.pos, flags);
        let i: usize = 0;
        while (i < inner_packet.len) {
            _.buf[_.pos + 4 + i] = inner_packet[i];
            i += 1;
        }
        _.pos += need;
        return true;
    }

    fn finishIpv4(*_): ([]const u8)? {
        if (!_.valid || _.pos < PIM_HEADER_SIZE) {
            return null;
        }
        writeU16Be(_.buf, 2, 0);
        const csum = pimChecksumIpv4(_.buf[0.._.pos]);
        writeU16Be(_.buf, 2, csum);
        return _.buf[0.._.pos];
    }

    fn finishIpv6(*_, src_ip: [16]u8, dst_ip: [16]u8): ([]const u8)? {
        if (!_.valid || _.pos < PIM_HEADER_SIZE) {
            return null;
        }
        writeU16Be(_.buf, 2, 0);
        const csum = pimChecksumIpv6(src_ip, dst_ip, _.buf[0.._.pos]);
        writeU16Be(_.buf, 2, csum);
        return _.buf[0.._.pos];
    }
}
