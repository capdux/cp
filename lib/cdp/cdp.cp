// Origin: Pure-CP Stdlib (Zero-Alloc Cisco Discovery Protocol v1/v2 Wire Codec) | Created: 2026-11-22

pub const CDP_SNAP_PID: u16 = 0x2000;
pub const CDP_V1: u8 = 1;
pub const CDP_V2: u8 = 2;
pub const CDP_HEADER_SIZE: usize = 4;
pub const CDP_TLV_HEADER_SIZE: usize = 4;

// Standard CDP TLV Types
pub const TLV_DEVICE_ID: u16 = 0x0001;
pub const TLV_ADDRESSES: u16 = 0x0002;
pub const TLV_PORT_ID: u16 = 0x0003;
pub const TLV_CAPABILITIES: u16 = 0x0004;
pub const TLV_VERSION: u16 = 0x0005;
pub const TLV_PLATFORM: u16 = 0x0006;
pub const TLV_VTP_DOMAIN: u16 = 0x0009;
pub const TLV_NATIVE_VLAN: u16 = 0x000A;
pub const TLV_DUPLEX: u16 = 0x000B;
pub const TLV_MGMT_ADDR: u16 = 0x0016;

// CDP Capability Bitmasks
pub const CAP_ROUTER: u32 = 0x0000_0001;
pub const CAP_TRANSPARENT_BRIDGE: u32 = 0x0000_0002;
pub const CAP_SOURCE_ROUTE_BRIDGE: u32 = 0x0000_0004;
pub const CAP_SWITCH: u32 = 0x0000_0008;
pub const CAP_HOST: u32 = 0x0000_0010;
pub const CAP_IGMP: u32 = 0x0000_0020;
pub const CAP_REPEATER: u32 = 0x0000_0040;

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

/// Computes RFC 1071 16-bit one's complement checksum for a CDP packet,
/// treating header checksum bytes [2..4] as 0x0000 and padding odd trailing byte with zero.
pub fn cdpChecksum(data: []const u8): u16 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i + 1 < data.len) {
        let hi: u32 = data[i] as u32;
        let lo: u32 = data[i + 1] as u32;
        if (i == 2) {
            hi = 0;
            lo = 0;
        }
        sum += (hi << 8) | lo;
        i += 2;
    }
    if (i < data.len) {
        let last: u32 = data[i] as u32;
        if (i == 2) {
            last = 0;
        }
        sum += last << 8;
    }
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ((sum ^ 0xFFFF) & 0xFFFF) as u16;
}

/// Verifies the Big-Endian 16-bit RFC 1071 checksum at bytes [2..4] of a CDP packet.
pub fn verifyCdpChecksum(data: []const u8): bool {
    if (data.len < CDP_HEADER_SIZE) {
        return false;
    }
    const stored = readU16Be(data, 2);
    return stored == cdpChecksum(data);
}

pub struct CdpHeader {
    version: u8,
    ttl: u8,
    checksum: u16,
}

pub struct CdpTlv {
    tlv_type: u16,
    length: u16,
    value: []const u8,
}

pub struct CdpTlvIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    fn init(tlv_bytes: []const u8): CdpTlvIter {
        return CdpTlvIter{
            .data = tlv_bytes,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): CdpTlv? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < CDP_TLV_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const tlv_type = readU16Be(_.data, _.offset);
        const length = readU16Be(_.data, _.offset + 2);
        const len_u = length as usize;
        if (len_u < CDP_TLV_HEADER_SIZE || len_u > avail) {
            _.failed = true;
            return null;
        }
        const val_start = _.offset + CDP_TLV_HEADER_SIZE;
        const val_end = _.offset + len_u;
        _.offset = val_end;
        return CdpTlv{
            .tlv_type = tlv_type,
            .length = length,
            .value = _.data[val_start..val_end],
        };
    }
}

pub struct CdpPacket {
    header: CdpHeader,
    tlv_bytes: []const u8,

    fn tlvs(): CdpTlvIter {
        return CdpTlvIter.init(_.tlv_bytes);
    }

    fn findTlv(tlv_type: u16): CdpTlv? {
        let it = _.tlvs();
        while (true) {
            if (tlv := it.next()) {
                if (tlv.tlv_type == tlv_type) {
                    return tlv;
                }
            } else {
                break;
            }
        }
        return null;
    }

    fn deviceId(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_DEVICE_ID)) {
            return tlv.value;
        }
        return null;
    }

    fn portId(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_PORT_ID)) {
            return tlv.value;
        }
        return null;
    }

    fn platform(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_PLATFORM)) {
            return tlv.value;
        }
        return null;
    }

    fn softwareVersion(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_VERSION)) {
            return tlv.value;
        }
        return null;
    }

    fn vtpDomain(): ([]const u8)? {
        if (tlv := _.findTlv(TLV_VTP_DOMAIN)) {
            return tlv.value;
        }
        return null;
    }

    fn capabilities(): u32? {
        if (tlv := _.findTlv(TLV_CAPABILITIES)) {
            if (tlv.value.len >= 4) {
                return readU32Be(tlv.value, 0);
            }
        }
        return null;
    }

    fn nativeVlan(): u16? {
        if (tlv := _.findTlv(TLV_NATIVE_VLAN)) {
            if (tlv.value.len >= 2) {
                return readU16Be(tlv.value, 0);
            }
        }
        return null;
    }

    fn isFullDuplex(): bool? {
        if (tlv := _.findTlv(TLV_DUPLEX)) {
            if (tlv.value.len >= 1) {
                return tlv.value[0] != 0;
            }
        }
        return null;
    }
}

/// Fast check for CDP v1/v2 header presence (len >= 4 and version == 1 or 2).
pub fn isCdp(data: []const u8): bool {
    if (data.len < CDP_HEADER_SIZE) {
        return false;
    }
    return data[0] == CDP_V1 || data[0] == CDP_V2;
}

/// Parses a CDP v1/v2 packet and strictly validates all TLV bounds (length >= 4 and no trailing truncation).
pub fn parseCdp(data: []const u8): CdpPacket? {
    if (!isCdp(data)) {
        return null;
    }
    const tlv_slice = data[CDP_HEADER_SIZE..];
    // Pre-validate all TLV boundaries
    let it = CdpTlvIter.init(tlv_slice);
    while (true) {
        if (tlv := it.next()) {
            * = tlv;
        } else {
            break;
        }
    }
    if (it.hasError()) {
        return null;
    }

    const hdr = CdpHeader{
        .version = data[0],
        .ttl = data[1],
        .checksum = readU16Be(data, 2),
    };
    return CdpPacket{
        .header = hdr,
        .tlv_bytes = tlv_slice,
    };
}

/// Zero-alloc CDP v1/v2 packet builder with automatic RFC 1071 checksum calculation.
pub struct CdpWriter {
    buf: []u8,
    pos: usize,
    valid: bool,

    fn init(buf: []u8, version: u8, ttl: u8): CdpWriter {
        if (buf.len < CDP_HEADER_SIZE || (version != CDP_V1 && version != CDP_V2)) {
            return CdpWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        buf[0] = version;
        buf[1] = ttl;
        buf[2] = 0;
        buf[3] = 0;
        return CdpWriter{
            .buf = buf,
            .pos = CDP_HEADER_SIZE,
            .valid = true,
        };
    }

    fn remaining(): usize {
        if (!_.valid || _.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeTlv(*_, tlv_type: u16, value: []const u8): bool {
        if (!_.valid) {
            return false;
        }
        const total = CDP_TLV_HEADER_SIZE + value.len;
        if (total > 65535 || _.remaining() < total) {
            return false;
        }
        writeU16Be(_.buf, _.pos, tlv_type);
        writeU16Be(_.buf, _.pos + 2, total as u16);
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + CDP_TLV_HEADER_SIZE + i] = value[i];
            i += 1;
        }
        _.pos += total;
        return true;
    }

    fn writeCapabilities(*_, caps: u32): bool {
        let raw: [4]u8 = [0, 0, 0, 0];
        writeU32Be(raw[0..], 0, caps);
        return _.writeTlv(TLV_CAPABILITIES, raw[0..]);
    }

    fn writeNativeVlan(*_, vlan: u16): bool {
        let raw: [2]u8 = [0, 0];
        writeU16Be(raw[0..], 0, vlan);
        return _.writeTlv(TLV_NATIVE_VLAN, raw[0..]);
    }

    fn writeDuplex(*_, full_duplex: bool): bool {
        const b: u8 = if (full_duplex) 1 else 0;
        const raw: [1]u8 = [b];
        return _.writeTlv(TLV_DUPLEX, raw[0..]);
    }

    fn finish(*_): ([]const u8)? {
        if (!_.valid || _.pos < CDP_HEADER_SIZE) {
            return null;
        }
        writeU16Be(_.buf, 2, 0);
        const csum = cdpChecksum(_.buf[0.._.pos]);
        writeU16Be(_.buf, 2, csum);
        return _.buf[0.._.pos];
    }
}
