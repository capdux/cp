// Origin: Pure-CP Stdlib (Zero-Alloc RFC 7348 VXLAN & VXLAN-GPE Codec) | Created: 2026-11-22

pub const VXLAN_HEADER_SIZE: usize = 8;
pub const VXLAN_DEFAULT_PORT: u16 = 4789;
pub const VXLAN_GPE_PORT: u16 = 4790;
pub const VXLAN_MAX_VNI: u32 = 0x00FF_FFFF;

pub const VXLAN_FLAG_I: u8 = 0x08;
pub const VXLAN_GPE_FLAG_P: u8 = 0x04;
pub const VXLAN_GPE_FLAG_O: u8 = 0x01;
pub const VXLAN_GBP_FLAG_G: u8 = 0x80;

pub enum VxlanNextProto: u8 {
    none = 0,
    ipv4 = 1,
    ipv6 = 2,
    ethernet = 3,
    nsh = 4,
    mpls = 5,
    unknown = 255,
}

pub fn vxlanNextProtoFromByte(b: u8): VxlanNextProto {
    if (b == 0) return VxlanNextProto.none;
    if (b == 1) return VxlanNextProto.ipv4;
    if (b == 2) return VxlanNextProto.ipv6;
    if (b == 3) return VxlanNextProto.ethernet;
    if (b == 4) return VxlanNextProto.nsh;
    if (b == 5) return VxlanNextProto.mpls;
    return VxlanNextProto.unknown;
}

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

fn writeU24Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 16) & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = (v & 0xFF) as u8;
}

pub struct VxlanHeader {
    flags: u8,
    vni: u32,
    next_proto: VxlanNextProto,
    group_policy_id: u16,

    fn hasValidVni(): bool {
        return (_.flags & VXLAN_FLAG_I) != 0;
    }

    fn isGpe(): bool {
        return (_.flags & VXLAN_GPE_FLAG_P) != 0;
    }

    fn isOam(): bool {
        return (_.flags & VXLAN_GPE_FLAG_O) != 0;
    }

    fn hasGbp(): bool {
        return (_.flags & VXLAN_GBP_FLAG_G) != 0;
    }
}

pub struct VxlanPacket {
    header: VxlanHeader,
    payload: []const u8,

    fn hasValidVni(): bool {
        return _.header.hasValidVni();
    }

    fn isGpe(): bool {
        return _.header.isGpe();
    }

    fn isOam(): bool {
        return _.header.isOam();
    }

    fn hasGbp(): bool {
        return _.header.hasGbp();
    }
}

/// Fast batch validation helper for VXLAN 8-byte header presence and I-bit.
pub fn vxlanVerifyHeader(data: []const u8): bool {
    if (data.len < VXLAN_HEADER_SIZE) {
        return false;
    }
    return (data[0] & VXLAN_FLAG_I) != 0;
}

/// Lenient RFC 7348 VXLAN parser: validates length >= 8 and I-bit, and decodes optional GPE / GBP fields.
pub fn parseVxlan(data: []const u8): VxlanPacket? {
    if (data.len < VXLAN_HEADER_SIZE) {
        return null;
    }
    const flags = data[0];
    if ((flags & VXLAN_FLAG_I) == 0) {
        return null;
    }

    let next_proto = VxlanNextProto.none;
    if ((flags & VXLAN_GPE_FLAG_P) != 0) {
        next_proto = vxlanNextProtoFromByte(data[3]);
    }

    let gbp_id: u16 = 0;
    if ((flags & VXLAN_GBP_FLAG_G) != 0) {
        gbp_id = readU16Be(data, 2);
    }

    const vni = readU24Be(data, 4);
    const hdr = VxlanHeader{
        .flags = flags,
        .vni = vni,
        .next_proto = next_proto,
        .group_policy_id = gbp_id,
    };
    return VxlanPacket{
        .header = hdr,
        .payload = data[VXLAN_HEADER_SIZE..],
    };
}

/// Strict RFC 7348 classic VXLAN parser: requires flags == 0x08 and all reserved bytes == 0.
pub fn parseVxlanStrict(data: []const u8): VxlanPacket? {
    if (data.len < VXLAN_HEADER_SIZE) {
        return null;
    }
    if (data[0] != VXLAN_FLAG_I) {
        return null;
    }
    if (data[1] != 0 || data[2] != 0 || data[3] != 0 || data[7] != 0) {
        return null;
    }
    const hdr = VxlanHeader{
        .flags = data[0],
        .vni = readU24Be(data, 4),
        .next_proto = VxlanNextProto.none,
        .group_policy_id = 0,
    };
    return VxlanPacket{
        .header = hdr,
        .payload = data[VXLAN_HEADER_SIZE..],
    };
}

/// VXLAN-GPE parser: requires I-bit and P-bit set, reserved version/bytes zero, and decodes next_proto at byte 3.
pub fn parseVxlanGpe(data: []const u8): VxlanPacket? {
    if (data.len < VXLAN_HEADER_SIZE) {
        return null;
    }
    const flags = data[0];
    if ((flags & VXLAN_FLAG_I) == 0 || (flags & VXLAN_GPE_FLAG_P) == 0) {
        return null;
    }
    // Ver bits (bits 4..5, mask 0x30) must be 0 in VXLAN-GPE v0, and reserved bytes 1, 2, 7 must be 0.
    if ((flags & 0x30) != 0 || data[1] != 0 || data[2] != 0 || data[7] != 0) {
        return null;
    }
    const hdr = VxlanHeader{
        .flags = flags,
        .vni = readU24Be(data, 4),
        .next_proto = vxlanNextProtoFromByte(data[3]),
        .group_policy_id = 0,
    };
    return VxlanPacket{
        .header = hdr,
        .payload = data[VXLAN_HEADER_SIZE..],
    };
}

/// Zero-alloc RFC 7348 classic VXLAN encoder. Returns total bytes written or null on invalid VNI / insufficient buffer.
pub fn encodeVxlan(vni: u32, inner_payload: []const u8, out: []u8): usize? {
    if (vni > VXLAN_MAX_VNI) {
        return null;
    }
    const total = VXLAN_HEADER_SIZE + inner_payload.len;
    if (out.len < total) {
        return null;
    }
    out[0] = VXLAN_FLAG_I;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;
    writeU24Be(out, 4, vni);
    out[7] = 0;

    let i: usize = 0;
    while (i < inner_payload.len) {
        out[VXLAN_HEADER_SIZE + i] = inner_payload[i];
        i += 1;
    }
    return total;
}

/// Zero-alloc VXLAN-GPE encoder. Sets I-bit, P-bit, optional OAM bit, and Next Protocol byte at offset 3.
pub fn encodeVxlanGpe(
    vni: u32,
    next_proto: VxlanNextProto,
    is_oam: bool,
    inner_payload: []const u8,
    out: []u8,
): usize? {
    if (vni > VXLAN_MAX_VNI) {
        return null;
    }
    const total = VXLAN_HEADER_SIZE + inner_payload.len;
    if (out.len < total) {
        return null;
    }
    let flags: u8 = VXLAN_FLAG_I | VXLAN_GPE_FLAG_P;
    if (is_oam) {
        flags = flags | VXLAN_GPE_FLAG_O;
    }
    out[0] = flags;
    out[1] = 0;
    out[2] = 0;
    out[3] = next_proto as u8;
    writeU24Be(out, 4, vni);
    out[7] = 0;

    let i: usize = 0;
    while (i < inner_payload.len) {
        out[VXLAN_HEADER_SIZE + i] = inner_payload[i];
        i += 1;
    }
    return total;
}

/// Zero-alloc VXLAN-GBP encoder. Sets G-bit and I-bit with 16-bit Big-Endian Group Policy ID at bytes [2..4].
pub fn encodeVxlanGbp(vni: u32, group_policy_id: u16, inner_payload: []const u8, out: []u8): usize? {
    if (vni > VXLAN_MAX_VNI) {
        return null;
    }
    const total = VXLAN_HEADER_SIZE + inner_payload.len;
    if (out.len < total) {
        return null;
    }
    out[0] = VXLAN_GBP_FLAG_G | VXLAN_FLAG_I;
    out[1] = 0;
    out[2] = ((group_policy_id >> 8) & 0xFF) as u8;
    out[3] = (group_policy_id & 0xFF) as u8;
    writeU24Be(out, 4, vni);
    out[7] = 0;

    let i: usize = 0;
    while (i < inner_payload.len) {
        out[VXLAN_HEADER_SIZE + i] = inner_payload[i];
        i += 1;
    }
    return total;
}
