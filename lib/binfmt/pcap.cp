// Reference-hardened against rusticata/pcap-parser (MIT OR Apache-2.0)
// Source: cp/lib/pcap/pcap.cp (Pure CP libpcap Container & L2-L4 Network Dissector + 16-Lane SIMD)
// Pure CP zero-copy libpcap (.pcap) reader/writer and Ethernet/IPv4/TCP/UDP protocol dissector.

const std = @import("std");

type PcapVec16u8 = @Vector(16, u8);

pub const PCAP_MAGIC_US: u32 = 0xa1b2c3d4;
pub const PCAP_MAGIC_US_SWAPPED: u32 = 0xd4c3b2a1;
pub const PCAP_MAGIC_NS: u32 = 0xa1b23c4d;
pub const PCAP_MAGIC_NS_SWAPPED: u32 = 0x4d3cb2a1;

pub const PCAP_VERSION_MAJOR: u16 = 2;
pub const PCAP_VERSION_MINOR: u16 = 4;
pub const PCAP_GLOBAL_HEADER_SIZE: usize = 24;
pub const PCAP_PACKET_HEADER_SIZE: usize = 16;
pub const DEFAULT_MAX_SNAPLEN: u32 = 1024 * 1024;
pub const PCAP_DEFAULT_MAX_PACKETS: usize = 1_000_000;

pub err PcapError {
    InvalidMagic,
    UnsupportedVersion,
    TruncatedHeader,
    TruncatedPacket,
    PacketTooLarge,
    TooManyPackets,
    InvalidEthernet,
    InvalidIpv4,
    InvalidTransport,
    OutOfMemory,
}

pub enum PcapEndian {
    Little,
    Big,
}

pub enum PcapPrecision {
    Microsecond,
    Nanosecond,
}

pub enum LinkType: u32 {
    Ethernet = 1,
    RawIp = 101,
    LinuxSll = 113,
}

pub struct PcapGlobalHeader {
    version_major: u16,
    version_minor: u16,
    snaplen: u32,
    link_type: u32,
    endian: PcapEndian,
    precision: PcapPrecision,
}

pub struct PcapPacket {
    ts_sec: u32,
    ts_subsec: u32,
    orig_len: u32,
    data: []const u8,
}

pub struct PcapValidationStats {
    header: PcapGlobalHeader,
    packet_count: usize,
    total_payload_bytes: usize,
}

pub struct PcapFile {
    header: PcapGlobalHeader,
    packets: []PcapPacket,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.packets.len > 0) {
            allocator.free(_.packets);
        }
        _.packets = &.{};
    }
}

pub struct EthernetFrame {
    dst_mac: [6]u8,
    src_mac: [6]u8,
    ether_type: u16,
    payload: []const u8,
}

pub struct Ipv4Packet {
    ihl: u8,
    protocol: u8,
    ttl: u8,
    total_len: u16,
    src_ip: [4]u8,
    dst_ip: [4]u8,
    payload: []const u8,
}

pub struct TcpSegment {
    src_port: u16,
    dst_port: u16,
    seq: u32,
    ack: u32,
    data_offset: u8,
    flags: u8,
    window: u16,
    payload: []const u8,
}

pub struct UdpDatagram {
    src_port: u16,
    dst_port: u16,
    length: u16,
    payload: []const u8,
}

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Primitives + Scalar Differential References
// ============================================================================

fn pcapLoadVec16(data: []const u8, off: usize): PcapVec16u8 {
    const v: PcapVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD packet payload byte copy (hot path for capture frame encoding/decoding).
pub fn simdPcapCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v = pcapLoadVec16(src, i);
        const arr: [16]u8 = v;
        @memcpy(dst[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference for `simdPcapCopyBytes` (differential verification).
pub fn scalarPcapCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD byte slice equality check for magic bytes & packet payloads.
pub fn simdPcapBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va = pcapLoadVec16(a, i);
        const vb = pcapLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdPcapBytesEqual` (differential verification).
pub fn scalarPcapBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Endian-Aware Helpers & Magic Detection
// ============================================================================

fn pcapReadU16(bytes: []const u8, endian: PcapEndian): u16 {
    const arr: [2]u8 = [bytes[0], bytes[1]];
    if (endian == .Little) {
        return std.mem.readInt(u16, &arr, .little);
    }
    return std.mem.readInt(u16, &arr, .big);
}

fn pcapReadU32(bytes: []const u8, endian: PcapEndian): u32 {
    const arr: [4]u8 = [bytes[0], bytes[1], bytes[2], bytes[3]];
    if (endian == .Little) {
        return std.mem.readInt(u32, &arr, .little);
    }
    return std.mem.readInt(u32, &arr, .big);
}

fn pcapAppendU16(buf: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u16, endian: PcapEndian): PcapError!void {
    let raw: [2]u8 = undefined;
    if (endian == .Little) {
        std.mem.writeInt(u16, &raw, val, .little);
    } else {
        std.mem.writeInt(u16, &raw, val, .big);
    }
    buf.appendSlice(allocator, &raw) catch { return PcapError.OutOfMemory; };
}

fn pcapAppendU32(buf: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u32, endian: PcapEndian): PcapError!void {
    let raw: [4]u8 = undefined;
    if (endian == .Little) {
        std.mem.writeInt(u32, &raw, val, .little);
    } else {
        std.mem.writeInt(u32, &raw, val, .big);
    }
    buf.appendSlice(allocator, &raw) catch { return PcapError.OutOfMemory; };
}

/// Checks whether the byte slice starts with one of the 4 standard libpcap magic numbers
/// (microsecond/nanosecond × little-endian/big-endian).
pub fn isPcap(bytes: []const u8): bool {
    if (bytes.len < 4) return false;
    const magic_us_be: [4]u8 = [0xa1, 0xb2, 0xc3, 0xd4];
    const magic_us_le: [4]u8 = [0xd4, 0xc3, 0xb2, 0xa1];
    const magic_ns_be: [4]u8 = [0xa1, 0xb2, 0x3c, 0x4d];
    const magic_ns_le: [4]u8 = [0x4d, 0x3c, 0xb2, 0xa1];
    const head = bytes[0..4];
    return simdPcapBytesEqual(head, magic_us_be[0..4])
        || simdPcapBytesEqual(head, magic_us_le[0..4])
        || simdPcapBytesEqual(head, magic_ns_be[0..4])
        || simdPcapBytesEqual(head, magic_ns_le[0..4]);
}

/// Zero-alloc parser for the 24-byte libpcap global header (all 4 magic numbers + version 2.4).
pub fn parsePcapGlobalHeader(bytes: []const u8): PcapError!PcapGlobalHeader {
    if (bytes.len < PCAP_GLOBAL_HEADER_SIZE) {
        return PcapError.TruncatedHeader;
    }

    const magic_be = std.mem.readInt(u32, bytes[0..4], .big);
    let endian: PcapEndian = .Little;
    let precision: PcapPrecision = .Microsecond;

    if (magic_be == PCAP_MAGIC_US_SWAPPED) {
        endian = .Little;
        precision = .Microsecond;
    } else if (magic_be == PCAP_MAGIC_US) {
        endian = .Big;
        precision = .Microsecond;
    } else if (magic_be == PCAP_MAGIC_NS_SWAPPED) {
        endian = .Little;
        precision = .Nanosecond;
    } else if (magic_be == PCAP_MAGIC_NS) {
        endian = .Big;
        precision = .Nanosecond;
    } else {
        return PcapError.InvalidMagic;
    }

    const version_major = pcapReadU16(bytes[4..6], endian);
    const version_minor = pcapReadU16(bytes[6..8], endian);
    if (version_major != PCAP_VERSION_MAJOR or version_minor != PCAP_VERSION_MINOR) {
        return PcapError.UnsupportedVersion;
    }

    const snaplen = pcapReadU32(bytes[16..20], endian);
    const link_type = pcapReadU32(bytes[20..24], endian);

    return PcapGlobalHeader {
        version_major: version_major,
        version_minor: version_minor,
        snaplen: snaplen,
        link_type: link_type,
        endian: endian,
        precision: precision,
    };
}

// ============================================================================
// PcapWriter (with 16-Lane SIMD Packet Payload Copy Hot Path)
// ============================================================================

pub struct PcapWriter {
    allocator: std.mem.Allocator,
    endian: PcapEndian,
    precision: PcapPrecision,
    snaplen: u32,
    link_type: u32,
    buf: std.ArrayList(u8),

    pub static fn init(
        allocator: std.mem.Allocator,
        endian: PcapEndian,
        precision: PcapPrecision,
        snaplen: u32,
        link_type: u32,
    ): PcapError!PcapWriter {
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(allocator);

        let magic: u32 = PCAP_MAGIC_US;
        if (precision == .Nanosecond) {
            magic = PCAP_MAGIC_NS;
        }
        pcapAppendU32(&buf, allocator, magic, endian)?;
        pcapAppendU16(&buf, allocator, PCAP_VERSION_MAJOR, endian)?;
        pcapAppendU16(&buf, allocator, PCAP_VERSION_MINOR, endian)?;
        pcapAppendU32(&buf, allocator, 0, endian)?;
        pcapAppendU32(&buf, allocator, 0, endian)?;
        pcapAppendU32(&buf, allocator, snaplen, endian)?;
        pcapAppendU32(&buf, allocator, link_type, endian)?;

        return PcapWriter {
            allocator: allocator,
            endian: endian,
            precision: precision,
            snaplen: snaplen,
            link_type: link_type,
            buf: buf,
        };
    }

    pub fn addPacket(
        *_,
        ts_sec: u32,
        ts_subsec: u32,
        orig_len: u32,
        data: []const u8,
    ): PcapError!void {
        if (data.len > _.snaplen) {
            return PcapError.PacketTooLarge;
        }
        const incl_len: u32 = @intCast(data.len);
        pcapAppendU32(&_.buf, _.allocator, ts_sec, _.endian)?;
        pcapAppendU32(&_.buf, _.allocator, ts_subsec, _.endian)?;
        pcapAppendU32(&_.buf, _.allocator, incl_len, _.endian)?;
        pcapAppendU32(&_.buf, _.allocator, orig_len, _.endian)?;
        if (data.len > 0) {
            const old_len = _.buf.items.len;
            _.buf.resize(_.allocator, old_len + data.len) catch { return PcapError.OutOfMemory; };
            simdPcapCopyBytes(_.buf.items[old_len .. old_len + data.len], data);
        }
    }

    pub fn finish(*_, allocator: std.mem.Allocator): PcapError![]u8 {
        return _.buf.toOwnedSlice(allocator) catch { return PcapError.OutOfMemory; };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }
}

// ============================================================================
// Zero-Alloc Strict Validator & Hardened Parsers
// ============================================================================

/// Zero-alloc strict validator for libpcap streams using default resource bounds
/// (`DEFAULT_MAX_SNAPLEN` and `PCAP_DEFAULT_MAX_PACKETS`).
pub fn validatePcapStrict(bytes: []const u8): PcapError!PcapValidationStats {
    return validatePcapStrictWithLimits(bytes, DEFAULT_MAX_SNAPLEN, PCAP_DEFAULT_MAX_PACKETS);
}

/// Zero-alloc strict validator for libpcap streams with explicit snaplen & packet count bounds.
/// Validates all 4 magic numbers, 24B global header, and every 16B packet header without overflow.
pub fn validatePcapStrictWithLimits(
    bytes: []const u8,
    max_snaplen: u32,
    max_packets: usize,
): PcapError!PcapValidationStats {
    const header = parsePcapGlobalHeader(bytes)?;
    if (header.snaplen > max_snaplen) {
        return PcapError.PacketTooLarge;
    }

    let pos: usize = PCAP_GLOBAL_HEADER_SIZE;
    let packet_count: usize = 0;
    let total_payload_bytes: usize = 0;

    while (pos < bytes.len) {
        if (packet_count >= max_packets) {
            return PcapError.TooManyPackets;
        }
        if (bytes.len - pos < PCAP_PACKET_HEADER_SIZE) {
            return PcapError.TruncatedPacket;
        }
        const incl_len = pcapReadU32(bytes[pos + 8 .. pos + 12], header.endian);
        pos += PCAP_PACKET_HEADER_SIZE;

        if (incl_len > header.snaplen or incl_len > max_snaplen) {
            return PcapError.PacketTooLarge;
        }
        const incl_usize: usize = @intCast(incl_len);
        if (bytes.len - pos < incl_usize) {
            return PcapError.TruncatedPacket;
        }

        pos += incl_usize;
        packet_count += 1;
        total_payload_bytes += incl_usize;
    }

    return PcapValidationStats {
        header: header,
        packet_count: packet_count,
        total_payload_bytes: total_payload_bytes,
    };
}

pub fn parsePcap(allocator: std.mem.Allocator, bytes: []const u8): PcapError!PcapFile {
    return parsePcapWithLimits(allocator, bytes, DEFAULT_MAX_SNAPLEN, PCAP_DEFAULT_MAX_PACKETS);
}

pub fn parsePcapWithMaxSnaplen(
    allocator: std.mem.Allocator,
    bytes: []const u8,
    max_snaplen: u32,
): PcapError!PcapFile {
    return parsePcapWithLimits(allocator, bytes, max_snaplen, PCAP_DEFAULT_MAX_PACKETS);
}

pub fn parsePcapWithLimits(
    allocator: std.mem.Allocator,
    bytes: []const u8,
    max_snaplen: u32,
    max_packets: usize,
): PcapError!PcapFile {
    const header = parsePcapGlobalHeader(bytes)?;
    if (header.snaplen > max_snaplen) {
        return PcapError.PacketTooLarge;
    }

    let packets = std.ArrayList(PcapPacket).empty;
    errdefer packets.deinit(allocator);

    let pos: usize = PCAP_GLOBAL_HEADER_SIZE;
    while (pos < bytes.len) {
        if (packets.items.len >= max_packets) {
            return PcapError.TooManyPackets;
        }
        if (bytes.len - pos < PCAP_PACKET_HEADER_SIZE) {
            return PcapError.TruncatedPacket;
        }
        const ts_sec = pcapReadU32(bytes[pos .. pos + 4], header.endian);
        const ts_subsec = pcapReadU32(bytes[pos + 4 .. pos + 8], header.endian);
        const incl_len = pcapReadU32(bytes[pos + 8 .. pos + 12], header.endian);
        const orig_len = pcapReadU32(bytes[pos + 12 .. pos + 16], header.endian);
        pos += PCAP_PACKET_HEADER_SIZE;

        if (incl_len > header.snaplen or incl_len > max_snaplen) {
            return PcapError.PacketTooLarge;
        }
        const incl_usize: usize = @intCast(incl_len);
        if (bytes.len - pos < incl_usize) {
            return PcapError.TruncatedPacket;
        }

        const pkt = PcapPacket {
            ts_sec: ts_sec,
            ts_subsec: ts_subsec,
            orig_len: orig_len,
            data: bytes[pos .. pos + incl_usize],
        };
        packets.append(allocator, pkt) catch { return PcapError.OutOfMemory; };
        pos += incl_usize;
    }

    const owned = packets.toOwnedSlice(allocator) catch { return PcapError.OutOfMemory; };
    return PcapFile {
        header: header,
        packets: owned,
    };
}

// ============================================================================
// L2-L4 Protocol Dissectors (Ethernet / IPv4 / TCP / UDP)
// ============================================================================

pub fn parseEthernet(frame: []const u8): PcapError!EthernetFrame {
    if (frame.len < 14) {
        return PcapError.InvalidEthernet;
    }

    let dst_mac: [6]u8 = undefined;
    let src_mac: [6]u8 = undefined;
    simdPcapCopyBytes(dst_mac[0..6], frame[0..6]);
    simdPcapCopyBytes(src_mac[0..6], frame[6..12]);

    let ether_type = std.mem.readInt(u16, frame[12..14], .big);
    let offset: usize = 14;

    while (ether_type == 0x8100 or ether_type == 0x88a8) {
        if (frame.len - offset < 4) {
            return PcapError.InvalidEthernet;
        }
        ether_type = pcapReadU16(frame[offset + 2 .. offset + 4], .Big);
        offset += 4;
    }

    return EthernetFrame {
        dst_mac: dst_mac,
        src_mac: src_mac,
        ether_type: ether_type,
        payload: frame[offset..],
    };
}

pub fn parseIpv4(bytes: []const u8): PcapError!Ipv4Packet {
    if (bytes.len < 20) {
        return PcapError.InvalidIpv4;
    }

    const version = bytes[0] >> 4;
    if (version != 4) {
        return PcapError.InvalidIpv4;
    }

    const ihl: u8 = (bytes[0] & 0x0F) * 4;
    if (ihl < 20 or bytes.len < ihl) {
        return PcapError.InvalidIpv4;
    }

    const total_len = std.mem.readInt(u16, bytes[2..4], .big);
    if (total_len < ihl or bytes.len < total_len) {
        return PcapError.InvalidIpv4;
    }

    const ttl = bytes[8];
    const protocol = bytes[9];
    let src_ip: [4]u8 = undefined;
    let dst_ip: [4]u8 = undefined;
    simdPcapCopyBytes(src_ip[0..4], bytes[12..16]);
    simdPcapCopyBytes(dst_ip[0..4], bytes[16..20]);

    const ihl_usize: usize = @intCast(ihl);
    const total_usize: usize = @intCast(total_len);

    return Ipv4Packet {
        ihl: ihl,
        protocol: protocol,
        ttl: ttl,
        total_len: total_len,
        src_ip: src_ip,
        dst_ip: dst_ip,
        payload: bytes[ihl_usize..total_usize],
    };
}

pub fn parseTcp(bytes: []const u8): PcapError!TcpSegment {
    if (bytes.len < 20) {
        return PcapError.InvalidTransport;
    }

    const src_port = std.mem.readInt(u16, bytes[0..2], .big);
    const dst_port = std.mem.readInt(u16, bytes[2..4], .big);
    const seq = std.mem.readInt(u32, bytes[4..8], .big);
    const ack = std.mem.readInt(u32, bytes[8..12], .big);
    const data_offset: u8 = (bytes[12] >> 4) * 4;
    if (data_offset < 20 or bytes.len < data_offset) {
        return PcapError.InvalidTransport;
    }

    const flags = bytes[13];
    const window = std.mem.readInt(u16, bytes[14..16], .big);
    const offset_usize: usize = @intCast(data_offset);

    return TcpSegment {
        src_port: src_port,
        dst_port: dst_port,
        seq: seq,
        ack: ack,
        data_offset: data_offset,
        flags: flags,
        window: window,
        payload: bytes[offset_usize..],
    };
}

pub fn parseUdp(bytes: []const u8): PcapError!UdpDatagram {
    if (bytes.len < 8) {
        return PcapError.InvalidTransport;
    }

    const src_port = std.mem.readInt(u16, bytes[0..2], .big);
    const dst_port = std.mem.readInt(u16, bytes[2..4], .big);
    const length = std.mem.readInt(u16, bytes[4..6], .big);
    if (length < 8 or bytes.len < length) {
        return PcapError.InvalidTransport;
    }

    const len_usize: usize = @intCast(length);

    return UdpDatagram {
        src_port: src_port,
        dst_port: dst_port,
        length: length,
        payload: bytes[8..len_usize],
    };
}
