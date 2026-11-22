// Origin: Pure-CP Stdlib (Zero-Alloc RFC 3032 MPLS Label Stack, RFC 4385 PW-CW & RFC 5586 G-ACh Parser) | Created: 2026-11-22

pub const MPLS_ETHERTYPE_UNICAST: u16 = 0x8847;
pub const MPLS_ETHERTYPE_MULTICAST: u16 = 0x8848;
pub const MPLS_LSE_SIZE: usize = 4;
pub const MPLS_MAX_STACK_DEPTH: usize = 16;
pub const MPLS_MAX_LABEL: u32 = 0x000F_FFFF;

// RFC 3032 / RFC 7274 Special-Purpose MPLS Labels (0..15)
pub const LABEL_IPV4_EXPLICIT_NULL: u32 = 0;
pub const LABEL_ROUTER_ALERT: u32 = 1;
pub const LABEL_IPV6_EXPLICIT_NULL: u32 = 2;
pub const LABEL_IMPLICIT_NULL: u32 = 3;
pub const LABEL_ENTROPY_INDICATOR: u32 = 7;
pub const LABEL_GAL: u32 = 13;
pub const LABEL_OAM_ALERT: u32 = 14;
pub const LABEL_EXTENSION: u32 = 15;

pub enum MplsPayloadKind: u8 {
    pw_cw = 0,
    gach = 1,
    ipv4 = 4,
    ipv6 = 6,
    unknown = 255,
}

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

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

/// 4-byte RFC 3032 MPLS Label Stack Entry (20-bit label, 3-bit TC, 1-bit BoS, 8-bit TTL).
pub struct MplsLabel {
    label: u32,
    tc: u8,
    bottom_of_stack: bool,
    ttl: u8,

    /// Returns true if label is in the RFC 3032/7274 reserved special-purpose range (0..15).
    fn isSpecial(): bool {
        return _.label <= 15;
    }

    fn encode(): u32 {
        return encodeLse(_);
    }
}

/// Decodes a 32-bit Big-Endian MPLS Label Stack Entry.
pub fn decodeLse(raw: u32): MplsLabel {
    return MplsLabel{
        .label = (raw >> 12) & MPLS_MAX_LABEL,
        .tc = ((raw >> 9) & 0x07) as u8,
        .bottom_of_stack = ((raw >> 8) & 0x01) != 0,
        .ttl = (raw & 0xFF) as u8,
    };
}

/// Encodes an MplsLabel into a 32-bit wire value.
pub fn encodeLse(l: MplsLabel): u32 {
    const bos_bit: u32 = if (l.bottom_of_stack) 0x0000_0100 else 0;
    return ((l.label & MPLS_MAX_LABEL) << 12) |
        (((l.tc & 0x07) as u32) << 9) |
        bos_bit |
        (l.ttl as u32);
}

pub struct MplsLabelIter {
    stack_bytes: []const u8,
    offset: usize,

    fn init(stack_bytes: []const u8): MplsLabelIter {
        return MplsLabelIter{
            .stack_bytes = stack_bytes,
            .offset = 0,
        };
    }

    fn remaining(): usize {
        if (_.offset >= _.stack_bytes.len) {
            return 0;
        }
        return (_.stack_bytes.len - _.offset) / MPLS_LSE_SIZE;
    }

    fn next(*_): MplsLabel? {
        if (_.offset + MPLS_LSE_SIZE > _.stack_bytes.len) {
            return null;
        }
        const raw = readU32Be(_.stack_bytes, _.offset);
        _.offset += MPLS_LSE_SIZE;
        return decodeLse(raw);
    }
}

/// Peeks the first 4-bit nibble of an MPLS payload to classify IPv4 (4), IPv6 (6), G-ACh (1), or PW-CW (0).
pub fn peekPayloadVersion(payload: []const u8): MplsPayloadKind {
    if (payload.len == 0) {
        return MplsPayloadKind.unknown;
    }
    const nibble = (payload[0] >> 4) & 0x0F;
    if (nibble == 0) return MplsPayloadKind.pw_cw;
    if (nibble == 1) return MplsPayloadKind.gach;
    if (nibble == 4) return MplsPayloadKind.ipv4;
    if (nibble == 6) return MplsPayloadKind.ipv6;
    return MplsPayloadKind.unknown;
}

pub struct MplsStack {
    label_count: usize,
    header_len: usize,
    stack_bytes: []const u8,
    payload: []const u8,

    fn labels(): MplsLabelIter {
        return MplsLabelIter.init(_.stack_bytes);
    }

    fn topLabel(): MplsLabel? {
        if (_.stack_bytes.len < MPLS_LSE_SIZE) {
            return null;
        }
        return decodeLse(readU32Be(_.stack_bytes, 0));
    }

    fn bottomLabel(): MplsLabel? {
        if (_.stack_bytes.len < MPLS_LSE_SIZE) {
            return null;
        }
        return decodeLse(readU32Be(_.stack_bytes, _.stack_bytes.len - MPLS_LSE_SIZE));
    }

    fn payloadKind(): MplsPayloadKind {
        return peekPayloadVersion(_.payload);
    }
}

/// Parses an RFC 3032 MPLS label stack until Bottom-of-Stack (S == 1).
/// Rejects truncated entries, missing BoS, or stacks exceeding MPLS_MAX_STACK_DEPTH (16).
pub fn parseMpls(data: []const u8): MplsStack? {
    if (data.len < MPLS_LSE_SIZE) {
        return null;
    }
    let count: usize = 0;
    let offset: usize = 0;
    while (count < MPLS_MAX_STACK_DEPTH) {
        if (offset + MPLS_LSE_SIZE > data.len) {
            return null;
        }
        const raw = readU32Be(data, offset);
        const lse = decodeLse(raw);
        count += 1;
        offset += MPLS_LSE_SIZE;
        if (lse.bottom_of_stack) {
            return MplsStack{
                .label_count = count,
                .header_len = offset,
                .stack_bytes = data[0..offset],
                .payload = data[offset..],
            };
        }
    }
    // Exceeded MPLS_MAX_STACK_DEPTH without encountering S == 1
    return null;
}

/// Zero-alloc encoder for an MPLS label stack + payload. Automatically sets S=0 on non-final labels and S=1 on final label.
pub fn encodeMplsStack(labels: []const MplsLabel, payload: []const u8, out: []u8): usize? {
    if (labels.len == 0 || labels.len > MPLS_MAX_STACK_DEPTH) {
        return null;
    }
    const hdr_len = labels.len * MPLS_LSE_SIZE;
    const total = hdr_len + payload.len;
    if (out.len < total) {
        return null;
    }
    let i: usize = 0;
    while (i < labels.len) {
        let l = labels[i];
        if (l.label > MPLS_MAX_LABEL) {
            return null;
        }
        l.bottom_of_stack = (i + 1 == labels.len);
        writeU32Be(out, i * MPLS_LSE_SIZE, encodeLse(l));
        i += 1;
    }
    let j: usize = 0;
    while (j < payload.len) {
        out[hdr_len + j] = payload[j];
        j += 1;
    }
    return total;
}

/// RFC 5586 Generic Associated Channel Header (G-ACh): 4B header starting with nibble 0x1.
pub struct MplsGach {
    version: u8,
    reserved: u8,
    channel_type: u16,
    payload: []const u8,
}

/// Parses a 4-byte RFC 5586 G-ACh header from an MPLS payload. Requires nibble == 0x1 and len >= 4.
pub fn parseMplsGach(payload: []const u8): MplsGach? {
    if (payload.len < 4) {
        return null;
    }
    if (((payload[0] >> 4) & 0x0F) != 0x01) {
        return null;
    }
    return MplsGach{
        .version = payload[0] & 0x0F,
        .reserved = payload[1],
        .channel_type = readU16Be(payload, 2),
        .payload = payload[4..],
    };
}

/// RFC 4385 Pseudowire Associated Control Word (PW-CW): 4B header starting with nibble 0x0.
pub struct MplsPwCw {
    flags: u8,
    frg: u8,
    length: u8,
    sequence_number: u16,
    payload: []const u8,
}

/// Parses a 4-byte RFC 4385 PW Control Word from an MPLS payload. Requires nibble == 0x0 and len >= 4.
pub fn parseMplsPwCw(payload: []const u8): MplsPwCw? {
    if (payload.len < 4) {
        return null;
    }
    if (((payload[0] >> 4) & 0x0F) != 0x00) {
        return null;
    }
    return MplsPwCw{
        .flags = payload[0] & 0x0F,
        .frg = (payload[1] >> 6) & 0x03,
        .length = payload[1] & 0x3F,
        .sequence_number = readU16Be(payload, 2),
        .payload = payload[4..],
    };
}
