// Source: cp/lib/pcap/pcap.cp (Pure CP libpcap Container & L2-L4 Network Dissector)
// Pure CP zero-copy libpcap (.pcap) reader/writer and Ethernet/IPv4/TCP/UDP protocol dissector.

const std = @import("std");

pub const PCAP_MAGIC_US: u32 = 0xa1b2c3d4;
pub const PCAP_MAGIC_US_SWAPPED: u32 = 0xd4c3b2a1;
pub const PCAP_MAGIC_NS: u32 = 0xa1b23c4d;
pub const PCAP_MAGIC_NS_SWAPPED: u32 = 0x4d3cb2a1;

pub const PCAP_VERSION_MAJOR: u16 = 2;
pub const PCAP_VERSION_MINOR: u16 = 4;
pub const DEFAULT_MAX_SNAPLEN: u32 = 1024 * 1024;

pub const PcapError = error {
    InvalidMagic,
    UnsupportedVersion,
    TruncatedHeader,
    TruncatedPacket,
    PacketTooLarge,
    InvalidEthernet,
    InvalidIpv4,
    InvalidTransport,
    OutOfMemory,
};

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

fn readU16(bytes: []const u8, endian: PcapEndian): u16 {
    const arr: *const [2]u8 = @ptrCast(bytes.ptr);
    if (endian == .Little) {
        return std.mem.readInt(u16, arr, .little);
    }
    return std.mem.readInt(u16, arr, .big);
}

fn readU32(bytes: []const u8, endian: PcapEndian): u32 {
    const arr: *const [4]u8 = @ptrCast(bytes.ptr);
    if (endian == .Little) {
        return std.mem.readInt(u32, arr, .little);
    }
    return std.mem.readInt(u32, arr, .big);
}

fn appendU16(buf: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u16, endian: PcapEndian): PcapError!void {
    let raw: [2]u8 = undefined;
    if (endian == .Little) {
        std.mem.writeInt(u16, &raw, val, .little);
    } else {
        std.mem.writeInt(u16, &raw, val, .big);
    }
    buf.appendSlice(allocator, &raw) catch return PcapError.OutOfMemory;
}

fn appendU32(buf: *std.ArrayList(u8), allocator: std.mem.Allocator, val: u32, endian: PcapEndian): PcapError!void {
    let raw: [4]u8 = undefined;
    if (endian == .Little) {
        std.mem.writeInt(u32, &raw, val, .little);
    } else {
        std.mem.writeInt(u32, &raw, val, .big);
    }
    buf.appendSlice(allocator, &raw) catch return PcapError.OutOfMemory;
}

pub struct PcapWriter {
    allocator: std.mem.Allocator,
    endian: PcapEndian,
    precision: PcapPrecision,
    snaplen: u32,
    link_type: u32,
    buf: std.ArrayList(u8),

    pub fn init(
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
        appendU32(&buf, allocator, magic, endian)?;
        appendU16(&buf, allocator, PCAP_VERSION_MAJOR, endian)?;
        appendU16(&buf, allocator, PCAP_VERSION_MINOR, endian)?;
        appendU32(&buf, allocator, 0, endian)?;
        appendU32(&buf, allocator, 0, endian)?;
        appendU32(&buf, allocator, snaplen, endian)?;
        appendU32(&buf, allocator, link_type, endian)?;

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
        appendU32(&_.buf, _.allocator, ts_sec, _.endian)?;
        appendU32(&_.buf, _.allocator, ts_subsec, _.endian)?;
        appendU32(&_.buf, _.allocator, incl_len, _.endian)?;
        appendU32(&_.buf, _.allocator, orig_len, _.endian)?;
        _.buf.appendSlice(_.allocator, data) catch return PcapError.OutOfMemory;
    }

    pub fn finish(*_, allocator: std.mem.Allocator): PcapError![]u8 {
        return _.buf.toOwnedSlice(allocator) catch return PcapError.OutOfMemory;
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }
}

pub fn parsePcap(allocator: std.mem.Allocator, bytes: []const u8): PcapError!PcapFile {
    return parsePcapWithMaxSnaplen(allocator, bytes, DEFAULT_MAX_SNAPLEN);
}

pub fn parsePcapWithMaxSnaplen(
    allocator: std.mem.Allocator,
    bytes: []const u8,
    max_snaplen: u32,
): PcapError!PcapFile {
    if (bytes.len < 24) {
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

    const version_major = readU16(bytes[4..6], endian);
    const version_minor = readU16(bytes[6..8], endian);
    if (version_major != PCAP_VERSION_MAJOR or version_minor != PCAP_VERSION_MINOR) {
        return PcapError.UnsupportedVersion;
    }

    const snaplen = readU32(bytes[16..20], endian);
    const link_type = readU32(bytes[20..24], endian);
    if (snaplen > max_snaplen) {
        return PcapError.PacketTooLarge;
    }

    const header = PcapGlobalHeader {
        version_major: version_major,
        version_minor: version_minor,
        snaplen: snaplen,
        link_type: link_type,
        endian: endian,
        precision: precision,
    };

    let packets = std.ArrayList(PcapPacket).empty;
    errdefer packets.deinit(allocator);

    let pos: usize = 24;
    while (pos < bytes.len) {
        if (bytes.len - pos < 16) {
            return PcapError.TruncatedPacket;
        }
        const ts_sec = readU32(bytes[pos .. pos + 4], endian);
        const ts_subsec = readU32(bytes[pos + 4 .. pos + 8], endian);
        const incl_len = readU32(bytes[pos + 8 .. pos + 12], endian);
        const orig_len = readU32(bytes[pos + 12 .. pos + 16], endian);
        pos += 16;

        if (incl_len > snaplen or incl_len > max_snaplen) {
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
        packets.append(allocator, pkt) catch return PcapError.OutOfMemory;
        pos += incl_usize;
    }

    const owned = packets.toOwnedSlice(allocator) catch return PcapError.OutOfMemory;
    return PcapFile {
        header: header,
        packets: owned,
    };
}

pub fn parseEthernet(frame: []const u8): PcapError!EthernetFrame {
    if (frame.len < 14) {
        return PcapError.InvalidEthernet;
    }

    let dst_mac: [6]u8 = undefined;
    let src_mac: [6]u8 = undefined;
    @memcpy(&dst_mac, frame[0..6]);
    @memcpy(&src_mac, frame[6..12]);

    let ether_type = std.mem.readInt(u16, frame[12..14], .big);
    let offset: usize = 14;

    while (ether_type == 0x8100 or ether_type == 0x88a8) {
        if (frame.len - offset < 4) {
            return PcapError.InvalidEthernet;
        }
        ether_type = readU16(frame[offset + 2 .. offset + 4], .Big);
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
    @memcpy(&src_ip, bytes[12..16]);
    @memcpy(&dst_ip, bytes[16..20]);

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
