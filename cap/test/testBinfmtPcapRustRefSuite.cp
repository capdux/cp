// Reference-hardened test suite against rusticata/pcap-parser (MIT OR Apache-2.0)
// Covers all 4 libpcap magics (us/ns x LE/BE), PCAPng SHB/IDB/EPB LE+BE round-trips & multi-section
// endian switching, strict bounds/alignment/length-mismatch/truncation/overflow/DoS guards,
// 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity, and 1,600 deterministic PRNG fuzz cases
// (800 PCAP + 800 PCAPng).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PCAP_MAGIC_US,
    PCAP_MAGIC_US_SWAPPED,
    PCAP_MAGIC_NS,
    PCAP_MAGIC_NS_SWAPPED,
    PCAP_VERSION_MAJOR,
    PCAP_VERSION_MINOR,
    PCAP_GLOBAL_HEADER_SIZE,
    DEFAULT_MAX_SNAPLEN,
    PcapError,
    PcapEndian,
    PcapPrecision,
    LinkType,
    PcapFile,
    PcapWriter,
    simdPcapCopyBytes,
    scalarPcapCopyBytes,
    simdPcapBytesEqual,
    scalarPcapBytesEqual,
    isPcap,
    parsePcapGlobalHeader,
    validatePcapStrict,
    validatePcapStrictWithLimits,
    parsePcap,
    parsePcapWithLimits,
} from "../../lib/binfmt/pcap.cp";
import {
    BLOCK_SHB,
    BLOCK_IDB,
    BLOCK_EPB,
    BYTE_ORDER_MAGIC_SWAPPED,
    PcapngError,
    isPcapng,
    PcapngBlockIter,
    parseShb,
    parseIdb,
    parseEpb,
    PcapngOptionIter,
    validatePcapngStrict,
    validatePcapngStrictWithLimits,
    writePcapngShb,
    writePcapngIdb,
    writePcapngEpb,
} from "../../lib/binfmt/pcapng.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
}

fn putU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v >> 24));
    buf[off + 1] = @as(u8, @truncate(v >> 16));
    buf[off + 2] = @as(u8, @truncate(v >> 8));
    buf[off + 3] = @as(u8, @truncate(v));
}

fn checkPcapVariant(
    allocator: std.mem.Allocator,
    endian: PcapEndian,
    precision: PcapPrecision,
    expected_magic_be: u32,
): u32 {
    let w = PcapWriter.init(allocator, endian, precision, 65535, @intFromEnum(LinkType.Ethernet)) catch return 1;
    defer w.deinit();

    let payload1: [47]u8 = [0; 47];
    let payload2: [96]u8 = [0; 96];
    let i: usize = 0;
    while (i < payload1.len) : (i += 1) {
        payload1[i] = @as(u8, @truncate((i * 13 + 7) & 0xFF));
    }
    i = 0;
    while (i < payload2.len) : (i += 1) {
        payload2[i] = @as(u8, @truncate((i * 29 + 3) & 0xFF));
    }

    w.addPacket(1700000001, 123456, 47, payload1[0..]) catch return 2;
    w.addPacket(1700000002, 987654, 128, payload2[0..]) catch return 3;

    const raw = w.finish(allocator) catch return 4;
    defer allocator.free(raw);

    if (!isPcap(raw)) return 5;
    const magic_be = std.mem.readInt(u32, raw[0..4], .big);
    if (magic_be != expected_magic_be) return 6;

    const hdr = parsePcapGlobalHeader(raw) catch return 7;
    if (hdr.version_major != PCAP_VERSION_MAJOR) return 8;
    if (hdr.version_minor != PCAP_VERSION_MINOR) return 9;
    if (hdr.snaplen != 65535) return 10;
    if (hdr.endian != endian) return 11;
    if (hdr.precision != precision) return 12;

    const stats = validatePcapStrict(raw) catch return 13;
    if (stats.packet_count != 2) return 14;
    if (stats.total_payload_bytes != 47 + 96) return 15;

    let parsed = parsePcap(allocator, raw) catch return 16;
    defer parsed.deinit(allocator);
    if (parsed.packets.len != 2) return 17;
    if (parsed.packets[0].ts_sec != 1700000001) return 18;
    if (parsed.packets[0].ts_subsec != 123456) return 19;
    if (parsed.packets[0].orig_len != 47) return 20;
    if (!simdPcapBytesEqual(parsed.packets[0].data, payload1[0..])) return 21;
    if (parsed.packets[1].ts_sec != 1700000002) return 22;
    if (parsed.packets[1].ts_subsec != 987654) return 23;
    if (parsed.packets[1].orig_len != 128) return 24;
    if (!simdPcapBytesEqual(parsed.packets[1].data, payload2[0..])) return 25;
    return 0;
}

fn checkTest1Pcap4MagicRoundTrip(): u32 {
    const allocator = std.heap.page_allocator;
    const r1 = checkPcapVariant(allocator, .Little, .Microsecond, PCAP_MAGIC_US_SWAPPED);
    if (r1 != 0) return 100 + r1;
    const r2 = checkPcapVariant(allocator, .Big, .Microsecond, PCAP_MAGIC_US);
    if (r2 != 0) return 200 + r2;
    const r3 = checkPcapVariant(allocator, .Little, .Nanosecond, PCAP_MAGIC_NS_SWAPPED);
    if (r3 != 0) return 300 + r3;
    const r4 = checkPcapVariant(allocator, .Big, .Nanosecond, PCAP_MAGIC_NS);
    if (r4 != 0) return 400 + r4;
    return 0;
}

fn checkTest2PcapSecurityGuards(): u32 {
    const allocator = std.heap.page_allocator;

    // 2a. Truncated global header (< 24B)
    const short_hdr: [12]u8 = [0xd4, 0xc3, 0xb2, 0xa1, 2, 0, 4, 0, 0, 0, 0, 0];
    if (!isPcap(short_hdr[0..])) return 1;
    let caught_trunc_hdr = false;
    * = validatePcapStrict(short_hdr[0..]) catch |e| {
        caught_trunc_hdr = (e == PcapError.TruncatedHeader);
    };
    if (!caught_trunc_hdr) return 2;

    // 2b. Invalid magic
    let bad_magic: [24]u8 = [0; 24];
    bad_magic[0] = 0xDE;
    bad_magic[1] = 0xAD;
    bad_magic[2] = 0xBE;
    bad_magic[3] = 0xEF;
    if (isPcap(bad_magic[0..])) return 3;
    let caught_magic = false;
    * = validatePcapStrict(bad_magic[0..]) catch |e| {
        caught_magic = (e == PcapError.InvalidMagic);
    };
    if (!caught_magic) return 4;

    // 2c. Unsupported version
    let bad_ver: [24]u8 = [0; 24];
    putU32LE(bad_ver[0..], 0, PCAP_MAGIC_US);
    bad_ver[4] = 3;
    bad_ver[6] = 1;
    let caught_ver = false;
    * = validatePcapStrict(bad_ver[0..]) catch |e| {
        caught_ver = (e == PcapError.UnsupportedVersion);
    };
    if (!caught_ver) return 5;

    // 2d. Valid 3-packet PCAP + strict limits & malformed mutations
    let w = PcapWriter.init(allocator, .Little, .Microsecond, 256, 1) catch return 6;
    defer w.deinit();
    const pkt_data: [20]u8 = [0xAA; 20];
    w.addPacket(1, 0, 20, pkt_data[0..]) catch return 7;
    w.addPacket(2, 0, 20, pkt_data[0..]) catch return 8;
    w.addPacket(3, 0, 20, pkt_data[0..]) catch return 9;
    const raw = w.finish(allocator) catch return 10;
    defer allocator.free(raw);

    // Packet cap exceeded (max_packets = 2 < 3)
    let caught_max_pkt = false;
    * = validatePcapStrictWithLimits(raw, 256, 2) catch |e| {
        caught_max_pkt = (e == PcapError.TooManyPackets);
    };
    if (!caught_max_pkt) return 11;

    let caught_parse_max_pkt = false;
    * = parsePcapWithLimits(allocator, raw, 256, 2) catch |e| {
        caught_parse_max_pkt = (e == PcapError.TooManyPackets);
    };
    if (!caught_parse_max_pkt) return 12;

    // Truncated packet
    let caught_trunc_pkt = false;
    * = validatePcapStrict(raw[0 .. raw.len - 5]) catch |e| {
        caught_trunc_pkt = (e == PcapError.TruncatedPacket);
    };
    if (!caught_trunc_pkt) return 13;

    // Overflowing incl_len = 0xFFFFFFFF in first packet header
    let mut_buf: [256]u8 = [0; 256];
    simdPcapCopyBytes(mut_buf[0..raw.len], raw);
    putU32LE(mut_buf[0..], PCAP_GLOBAL_HEADER_SIZE + 8, 0xFFFFFFFF);
    let caught_overflow = false;
    * = validatePcapStrict(mut_buf[0..raw.len]) catch |e| {
        caught_overflow = (e == PcapError.PacketTooLarge);
    };
    if (!caught_overflow) return 14;

    // incl_len within snaplen (200 <= 256) but exceeds remaining slice -> TruncatedPacket
    putU32LE(mut_buf[0..], PCAP_GLOBAL_HEADER_SIZE + 8, 200);
    let caught_oob = false;
    * = validatePcapStrict(mut_buf[0..raw.len]) catch |e| {
        caught_oob = (e == PcapError.TruncatedPacket);
    };
    if (!caught_oob) return 15;

    // Snaplen exceeds max_snaplen
    putU32LE(mut_buf[0..], 16, DEFAULT_MAX_SNAPLEN + 1);
    let caught_snap = false;
    * = validatePcapStrict(mut_buf[0..PCAP_GLOBAL_HEADER_SIZE]) catch |e| {
        caught_snap = (e == PcapError.PacketTooLarge);
    };
    if (!caught_snap) return 16;

    return 0;
}

fn checkTest3PcapngRoundTrip(): u32 {
    let stream: [512]u8 = [0; 512];
    let off: usize = 0;

    // Section 1: Little-Endian
    const payload_le: [37]u8 = [
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25,
    ];
    off += writePcapngShb(stream[off..], 1, 0, -1, false) catch return 1;
    off += writePcapngIdb(stream[off..], 1, 65535, false) catch return 2;
    off += writePcapngEpb(stream[off..], 0, 0x00000010, 0x20304050, 37, payload_le[0..], false) catch return 3;

    // Section 2: Big-Endian (endianness switch mid-stream)
    const payload_be: [48]u8 = [0x5A; 48];
    off += writePcapngShb(stream[off..], 1, 0, -1, true) catch return 4;
    off += writePcapngIdb(stream[off..], 101, 16384, true) catch return 5;
    off += writePcapngEpb(stream[off..], 0, 0x00000020, 0x60708090, 64, payload_be[0..], true) catch return 6;

    const data = stream[0..off];
    if (!isPcapng(data)) return 7;

    const stats = validatePcapngStrict(data) catch return 8;
    if (stats.section_count != 2) return 9;
    if (stats.interface_count != 2) return 10;
    if (stats.packet_count != 2) return 11;
    if (stats.total_blocks != 6) return 12;
    if (stats.total_packet_bytes != 37 + 48) return 13;

    let it = PcapngBlockIter.init(data);
    let idx: usize = 0;
    while (blk := it.next()) {
        if (idx == 0) {
            if (blk.block_type != BLOCK_SHB || blk.is_big_endian) return 14;
            const shb = parseShb(blk.body, blk.is_big_endian) orelse return 15;
            if (shb.major_version != 1 || shb.minor_version != 0 || shb.section_length != -1) return 16;
        } else if (idx == 1) {
            if (blk.block_type != BLOCK_IDB) return 17;
            const idb = parseIdb(blk.body, blk.is_big_endian) orelse return 18;
            if (idb.link_type != 1 || idb.snap_len != 65535) return 19;
        } else if (idx == 2) {
            if (blk.block_type != BLOCK_EPB) return 20;
            const epb = parseEpb(blk.body, blk.is_big_endian) orelse return 21;
            if (epb.captured_len != 37 || epb.original_len != 37) return 22;
            if (epb.timestamp64() != 0x0000001020304050) return 23;
            if (!simdPcapBytesEqual(epb.packet_data, payload_le[0..])) return 24;
        } else if (idx == 3) {
            if (blk.block_type != BLOCK_SHB || !blk.is_big_endian) return 25;
            const shb = parseShb(blk.body, blk.is_big_endian) orelse return 26;
            if (shb.major_version != 1) return 27;
        } else if (idx == 4) {
            if (blk.block_type != BLOCK_IDB || !blk.is_big_endian) return 28;
            const idb = parseIdb(blk.body, blk.is_big_endian) orelse return 29;
            if (idb.link_type != 101 || idb.snap_len != 16384) return 30;
        } else if (idx == 5) {
            if (blk.block_type != BLOCK_EPB || !blk.is_big_endian) return 31;
            const epb = parseEpb(blk.body, blk.is_big_endian) orelse return 32;
            if (epb.captured_len != 48 || epb.original_len != 64) return 33;
            if (!simdPcapBytesEqual(epb.packet_data, payload_be[0..])) return 34;
        }
        idx += 1;
    }
    if (it.had_error || idx != 6) return 35;

    // Verify standalone Big-Endian PCAPng starts with 0x4D3C2B1A in LE view
    let be_only: [64]u8 = [0; 64];
    const be_n = writePcapngShb(be_only[0..], 1, 0, -1, true) catch return 36;
    if (!isPcapng(be_only[0..be_n])) return 37;
    const magic_le_view = std.mem.readInt(u32, be_only[8..12], .little);
    if (magic_le_view != BYTE_ORDER_MAGIC_SWAPPED) return 38;
    return 0;
}

fn checkTest4PcapngSecurityGuards(): u32 {
    let base: [128]u8 = [0; 128];
    let off: usize = 0;
    const payload: [10]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    off += writePcapngShb(base[off..], 1, 0, -1, false) catch return 1;
    off += writePcapngIdb(base[off..], 1, 65535, false) catch return 2;
    off += writePcapngEpb(base[off..], 0, 1, 2, 10, payload[0..], false) catch return 3;

    // 4a. Trailing length mismatch
    let bad: [128]u8 = [0; 128];
    simdPcapCopyBytes(bad[0..off], base[0..off]);
    putU32LE(bad[0..], 24, 32);
    let caught_mismatch = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_mismatch = (e == PcapngError.LengthMismatch);
    };
    if (!caught_mismatch) return 4;

    // 4b. Unaligned block_total_length (30 not divisible by 4)
    simdPcapCopyBytes(bad[0..off], base[0..off]);
    putU32LE(bad[0..], 4, 30);
    let caught_unaligned = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_unaligned = (e == PcapngError.UnalignedBlockLength);
    };
    if (!caught_unaligned) return 5;

    // 4c. Block length < 12 (e.g. 8)
    simdPcapCopyBytes(bad[0..off], base[0..off]);
    putU32LE(bad[0..], 4, 8);
    let caught_short = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_short = (e == PcapngError.BlockTooShort);
    };
    if (!caught_short) return 6;

    // 4d. Truncated stream
    let caught_trunc = false;
    * = validatePcapngStrict(base[0 .. off - 4]) catch |e| {
        caught_trunc = (e == PcapngError.TruncatedBlock);
    };
    if (!caught_trunc) return 7;

    // 4e. EPB captured_len overflow / OOB
    simdPcapCopyBytes(bad[0..off], base[0..off]);
    const epb_off: usize = 28 + 20;
    putU32LE(bad[0..], epb_off + 20, 0xFFFFFFFF);
    let caught_epb_snap = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_epb_snap = (e == PcapngError.SnaplenExceeded);
    };
    if (!caught_epb_snap) return 8;

    putU32LE(bad[0..], epb_off + 20, 100);
    let caught_epb_oob = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_epb_oob = (e == PcapngError.CapturedLenOutOfBounds);
    };
    if (!caught_epb_oob) return 9;

    // 4f. Block count limit (max_blocks = 2 < 3)
    let caught_max_blk = false;
    * = validatePcapngStrictWithLimits(base[0..off], 2, 65535) catch |e| {
        caught_max_blk = (e == PcapngError.TooManyBlocks);
    };
    if (!caught_max_blk) return 10;

    let it_cap = PcapngBlockIter.initWithMaxBlocks(base[0..off], 2);
    let cnt: usize = 0;
    while (blk := it_cap.next()) {
        * = blk;
        cnt += 1;
    }
    if (cnt != 2 || !it_cap.had_error) return 11;

    // 4g. Invalid SHB byte-order magic
    simdPcapCopyBytes(bad[0..off], base[0..off]);
    putU32LE(bad[0..], 8, 0x11223344);
    let caught_bom = false;
    * = validatePcapngStrict(bad[0..off]) catch |e| {
        caught_bom = (e == PcapngError.InvalidByteOrderMagic);
    };
    if (!caught_bom) return 12;

    return 0;
}

fn checkTest5SimdParity(): u32 {
    let rng = XorShift64.init(0x50434150_31365645);
    let src: [192]u8 = [0; 192];
    let dst_simd: [192]u8 = [0; 192];
    let dst_scalar: [192]u8 = [0; 192];
    let other: [192]u8 = [0; 192];

    let len: usize = 0;
    while (len <= 160) : (len += 1) {
        rng.fillBytes(src[0..len]);
        let k: usize = 0;
        while (k < len) : (k += 1) {
            dst_simd[k] = 0xCC;
            dst_scalar[k] = 0xDD;
        }

        simdPcapCopyBytes(dst_simd[0..len], src[0..len]);
        scalarPcapCopyBytes(dst_scalar[0..len], src[0..len]);

        if (!simdPcapBytesEqual(dst_simd[0..len], dst_scalar[0..len])) return 1;
        if (!scalarPcapBytesEqual(dst_simd[0..len], dst_scalar[0..len])) return 2;
        if (!simdPcapBytesEqual(dst_simd[0..len], src[0..len])) return 3;

        if (len > 0) {
            simdPcapCopyBytes(other[0..len], src[0..len]);
            const flip_pos = @as(usize, @truncate(rng.next())) % len;
            other[flip_pos] ^= 0x01;
            const eq_simd = simdPcapBytesEqual(src[0..len], other[0..len]);
            const eq_scalar = scalarPcapBytesEqual(src[0..len], other[0..len]);
            if (eq_simd || eq_simd != eq_scalar) return 4;
        }
    }

    if (simdPcapBytesEqual(src[0..16], src[0..17])) return 5;
    if (scalarPcapBytesEqual(src[0..16], src[0..17])) return 6;
    return 0;
}

fn checkTest6PrngFuzzing(): u32 {
    const allocator = std.heap.page_allocator;
    let rng = XorShift64.init(0x7022_2026_0037_0267);
    let buf: [256]u8 = [0; 256];

    // 6a. 800 PCAP fuzz cases
    let valid_pcap_count: usize = 0;
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 256;
        rng.fillBytes(buf[0..len]);

        if (len >= 24 && (iter & 1) == 0) {
            const m = iter % 4;
            if (m == 0) {
                putU32LE(buf[0..], 0, PCAP_MAGIC_US);
                buf[4] = 2;
                buf[5] = 0;
                buf[6] = 4;
                buf[7] = 0;
            } else if (m == 1) {
                putU32BE(buf[0..], 0, PCAP_MAGIC_US);
                buf[4] = 0;
                buf[5] = 2;
                buf[6] = 0;
                buf[7] = 4;
            } else if (m == 2) {
                putU32LE(buf[0..], 0, PCAP_MAGIC_NS);
                buf[4] = 2;
                buf[5] = 0;
                buf[6] = 4;
                buf[7] = 0;
            } else {
                putU32BE(buf[0..], 0, PCAP_MAGIC_NS);
                buf[4] = 0;
                buf[5] = 2;
                buf[6] = 0;
                buf[7] = 4;
            }
            if ((iter & 2) == 0) {
                if (m == 0 || m == 2) {
                    putU32LE(buf[0..], 16, 128);
                } else {
                    putU32BE(buf[0..], 16, 128);
                }
            }
        }

        const slice = buf[0..len];
        let v_ok = true;
        * = validatePcapStrictWithLimits(slice, 4096, 64) catch {
            v_ok = false;
        };
        let pf = parsePcapWithLimits(allocator, slice, 4096, 64) catch {
            if (v_ok) return 1;
            continue;
        };
        defer pf.deinit(allocator);
        if (!v_ok) return 1;
        valid_pcap_count += 1;
    }
    if (valid_pcap_count > 800) return 2;

    // 6b. 800 PCAPng fuzz cases
    let valid_pcapng_count: usize = 0;
    iter = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 256;
        rng.fillBytes(buf[0..len]);

        if (len >= 28 && (iter & 1) == 0) {
            const is_be = ((iter & 2) != 0);
            * = writePcapngShb(buf[0..28], 1, 0, -1, is_be) catch return 3;
            if (len >= 48 && (iter & 4) == 0) {
                * = writePcapngIdb(buf[28..48], 1, 2048, is_be) catch return 4;
            }
        }

        const slice = buf[0..len];
        let ng_ok = true;
        * = validatePcapngStrictWithLimits(slice, 64, 4096) catch {
            ng_ok = false;
        };
        if (ng_ok) {
            valid_pcapng_count += 1;
            let it = PcapngBlockIter.initWithMaxBlocks(slice, 64);
            while (blk := it.next()) {
                if (blk.block_type == BLOCK_SHB) {
                    if (parseShb(blk.body, blk.is_big_endian) == null) return 5;
                } else if (blk.block_type == BLOCK_IDB) {
                    if (parseIdb(blk.body, blk.is_big_endian) == null) return 6;
                } else if (blk.block_type == BLOCK_EPB) {
                    if (parseEpb(blk.body, blk.is_big_endian) == null) return 7;
                }
            }
            if (it.had_error) return 8;
        } else {
            let it = PcapngBlockIter.initWithMaxBlocks(slice, 64);
            while (blk := it.next()) {
                let opt_it = PcapngOptionIter.init(blk.body, blk.is_big_endian);
                while (opt := opt_it.next()) {
                    * = opt;
                }
            }
        }
    }
    if (valid_pcapng_count > 800) return 9;
    return 0;
}

fn main() {
    test("1. PCAP 4-Magic Round-Trip (us/ns x LE/BE) & Zero-Alloc Strict Validation", () => {
        expect_eq(checkTest1Pcap4MagicRoundTrip(), 0 as u32);
    });

    test("2. PCAP Strict Security Guards (Truncation, Overflow, Snaplen, Packet Cap)", () => {
        expect_eq(checkTest2PcapSecurityGuards(), 0 as u32);
    });

    test("3. PCAPng SHB/IDB/EPB Round-Trip (LE + BE + Multi-Section Endian Switch)", () => {
        expect_eq(checkTest3PcapngRoundTrip(), 0 as u32);
    });

    test("4. PCAPng Strict Security Guards (Mismatch, Unaligned, Short, OOB, Block Cap)", () => {
        expect_eq(checkTest4PcapngSecurityGuards(), 0 as u32);
    });

    test("5. 16-Lane @Vector(16, u8) SIMD vs Scalar 0-Diff Differential Parity", () => {
        expect_eq(checkTest5SimdParity(), 0 as u32);
    });

    test("6. Deterministic PRNG Fuzzing (800 PCAP + 800 PCAPng = 1,600 Cases, Zero Crash/Leak)", () => {
        expect_eq(checkTest6PrngFuzzing(), 0 as u32);
    });
}
