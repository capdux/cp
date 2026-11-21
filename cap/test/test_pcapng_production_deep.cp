// Source: cp/cap/test/test_pcapng_production_deep.cp (Production deep tests for cap:pcapng)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BLOCK_SHB,
    BLOCK_IDB,
    BLOCK_SPB,
    BLOCK_NRB,
    BLOCK_ISB,
    BLOCK_EPB,
    BYTE_ORDER_MAGIC,
    PcapngBlock,
    ShbInfo,
    IdbInfo,
    EpbPacket,
    PcapngOption,
    isPcapng,
    PcapngBlockIter,
    parseShb,
    parseIdb,
    parseEpb,
    PcapngOptionIter,
} from "cap:pcapng";

fn putU16(buf: []u8, off: usize, v: u16, is_be: bool) {
    if (is_be) {
        buf[off] = ((v >> 8) & 0xFF) as u8;
        buf[off + 1] = (v & 0xFF) as u8;
    } else {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    }
}

fn putU32(buf: []u8, off: usize, v: u32, is_be: bool) {
    if (is_be) {
        buf[off] = ((v >> 24) & 0xFF) as u8;
        buf[off + 1] = ((v >> 16) & 0xFF) as u8;
        buf[off + 2] = ((v >> 8) & 0xFF) as u8;
        buf[off + 3] = (v & 0xFF) as u8;
    } else {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    }
}

fn putI64(buf: []u8, off: usize, v: i64, is_be: bool) {
    const raw: u64 = @bitCast(v);
    const hi: u32 = ((raw >> 32) & 0xFFFFFFFF) as u32;
    const lo: u32 = (raw & 0xFFFFFFFF) as u32;
    if (is_be) {
        putU32(buf, off, hi, true);
        putU32(buf, off + 4, lo, true);
    } else {
        putU32(buf, off, lo, false);
        putU32(buf, off + 4, hi, false);
    }
}

fn wrapBlock(dst: []u8, off: usize, btype: u32, body: []const u8, is_be: bool): usize {
    const total: u32 = (body.len + 12) as u32;
    putU32(dst, off, btype, is_be);
    putU32(dst, off + 4, total, is_be);
    let i: usize = 0;
    while (i < body.len) {
        dst[off + 8 + i] = body[i];
        i += 1;
    }
    putU32(dst, off + 8 + body.len, total, is_be);
    return body.len + 12;
}

test("1. Little-Endian PCAPng stream parses SHB + IDB + EPB with 64-bit timestamp", () => {
    let shb_body: [16]u8 = undefined;
    putU32(shb_body[0..16], 0, BYTE_ORDER_MAGIC, false);
    putU16(shb_body[0..16], 4, 1, false);
    putU16(shb_body[0..16], 6, 0, false);
    putI64(shb_body[0..16], 8, -1, false);

    let idb_body: [8]u8 = undefined;
    putU16(idb_body[0..8], 0, 1, false); // Ethernet
    putU16(idb_body[0..8], 2, 0, false);
    putU32(idb_body[0..8], 4, 65535, false);

    // EPB body: 20 header + 6 packet bytes + 2 padding bytes = 28 bytes
    let epb_body: [28]u8 = undefined;
    let i: usize = 0;
    while (i < 28) {
        epb_body[i] = 0;
        i += 1;
    }
    putU32(epb_body[0..28], 0, 0, false);          // interface_id = 0
    putU32(epb_body[0..28], 4, 0x00000001, false); // ts_hi
    putU32(epb_body[0..28], 8, 0x23456789, false); // ts_lo
    putU32(epb_body[0..28], 12, 6, false);         // captured_len
    putU32(epb_body[0..28], 16, 64, false);        // original_len
    epb_body[20] = 0xDE;
    epb_body[21] = 0xAD;
    epb_body[22] = 0xBE;
    epb_body[23] = 0xEF;
    epb_body[24] = 0xCA;
    epb_body[25] = 0xFE;

    let cap_buf: [128]u8 = undefined;
    const s = cap_buf[0..128];
    let pos: usize = 0;
    pos += wrapBlock(s, pos, BLOCK_SHB, shb_body[0..16], false);
    pos += wrapBlock(s, pos, BLOCK_IDB, idb_body[0..8], false);
    pos += wrapBlock(s, pos, BLOCK_EPB, epb_body[0..28], false);

    expect_eq(isPcapng(s[0..pos]), true);
    let it = PcapngBlockIter.init(s[0..pos]);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.block_type, BLOCK_SHB);
    expect_eq(b0.is_big_endian, false);
    const shb = parseShb(b0.body, b0.is_big_endian) ?? unreachable;
    expect_eq(shb.major_version, 1);
    expect_eq(shb.minor_version, 0);
    expect_eq(shb.section_length, -1);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.block_type, BLOCK_IDB);
    const idb = parseIdb(b1.body, b1.is_big_endian) ?? unreachable;
    expect_eq(idb.link_type, 1);
    expect_eq(idb.snap_len, 65535);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.block_type, BLOCK_EPB);
    const pkt = parseEpb(b2.body, b2.is_big_endian) ?? unreachable;
    expect_eq(pkt.interface_id, 0);
    expect_eq(pkt.timestamp64(), 0x0000000123456789);
    expect_eq(pkt.captured_len, 6);
    expect_eq(pkt.original_len, 64);
    expect_eq(pkt.packet_data.len, 6);
    expect_eq(pkt.packet_data[0], 0xDE);
    expect_eq(pkt.packet_data[5], 0xFE);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. Big-Endian PCAPng stream auto-detects endianness from SHB and decodes IDB/EPB", () => {
    let shb_body: [16]u8 = undefined;
    putU32(shb_body[0..16], 0, BYTE_ORDER_MAGIC, true);
    putU16(shb_body[0..16], 4, 1, true);
    putU16(shb_body[0..16], 6, 2, true);
    putI64(shb_body[0..16], 8, 4096, true);

    let idb_body: [8]u8 = undefined;
    putU16(idb_body[0..8], 0, 101, true); // Raw IP
    putU16(idb_body[0..8], 2, 0, true);
    putU32(idb_body[0..8], 4, 262144, true);

    let cap_buf: [64]u8 = undefined;
    const s = cap_buf[0..64];
    let pos: usize = 0;
    pos += wrapBlock(s, pos, BLOCK_SHB, shb_body[0..16], true);
    pos += wrapBlock(s, pos, BLOCK_IDB, idb_body[0..8], true);

    expect_eq(isPcapng(s[0..pos]), true);
    let it = PcapngBlockIter.init(s[0..pos]);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.is_big_endian, true);
    const shb = parseShb(b0.body, b0.is_big_endian) ?? unreachable;
    expect_eq(shb.major_version, 1);
    expect_eq(shb.minor_version, 2);
    expect_eq(shb.section_length, 4096);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.is_big_endian, true);
    const idb = parseIdb(b1.body, b1.is_big_endian) ?? unreachable;
    expect_eq(idb.link_type, 101);
    expect_eq(idb.snap_len, 262144);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. PcapngOptionIter iterates 4-byte aligned TLV options and stops at code 0 (endofopt)", () => {
    // Opt 1: code=2 (shb_hardware), len=5 ("Linux"), padded to 8 -> 12 bytes
    // Opt 2: code=4 (shb_userappl), len=3 ("cap"), padded to 4 -> 8 bytes
    // Opt 3: code=0 (endofopt), len=0 -> 4 bytes
    // Total = 24 bytes
    let opts: [24]u8 = undefined;
    let i: usize = 0;
    while (i < 24) {
        opts[i] = 0;
        i += 1;
    }
    let s = opts[0..24];
    putU16(s, 0, 2, false);
    putU16(s, 2, 5, false);
    s[4] = 'L';
    s[5] = 'i';
    s[6] = 'n';
    s[7] = 'u';
    s[8] = 'x';

    putU16(s, 12, 4, false);
    putU16(s, 14, 3, false);
    s[16] = 'c';
    s[17] = 'a';
    s[18] = 'p';

    putU16(s, 20, 0, false); // endofopt
    putU16(s, 22, 0, false);

    let it = PcapngOptionIter.init(s, false);
    const o0 = it.next() ?? unreachable;
    expect_eq(o0.code, 2);
    expect_eq(std.mem.eql(u8, o0.value, "Linux"), true);

    const o1 = it.next() ?? unreachable;
    expect_eq(o1.code, 4);
    expect_eq(std.mem.eql(u8, o1.value, "cap"), true);

    expect_eq(it.next() == null, true);
});

test("4. PcapngBlockIter rejects mismatched trailing length, unaligned block size, and truncated blocks", () => {
    let shb_body: [16]u8 = undefined;
    putU32(shb_body[0..16], 0, BYTE_ORDER_MAGIC, false);
    putU16(shb_body[0..16], 4, 1, false);
    putU16(shb_body[0..16], 6, 0, false);
    putI64(shb_body[0..16], 8, -1, false);

    let buf: [28]u8 = undefined;
    const s = buf[0..28];
    const len = wrapBlock(s, 0, BLOCK_SHB, shb_body[0..16], false);
    expect_eq(len, 28);

    // Corrupt trailing block_total_length
    putU32(s, 24, 32, false);
    let it1 = PcapngBlockIter.init(s);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // Non-4-byte aligned block_total_length (26)
    putU32(s, 4, 26, false);
    putU32(s, 24, 26, false);
    let it2 = PcapngBlockIter.init(s);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);

    // Truncated buffer
    putU32(s, 4, 28, false);
    putU32(s, 24, 28, false);
    let it3 = PcapngBlockIter.init(s[0..20]);
    expect_eq(it3.next() == null, true);
    expect_eq(it3.had_error, true);
});

test("5. parseEpb guards against out-of-bounds captured_len and isPcapng rejects non-PCAPng magic", () => {
    let epb_bad: [24]u8 = undefined;
    let i: usize = 0;
    while (i < 24) {
        epb_bad[i] = 0;
        i += 1;
    }
    putU32(epb_bad[0..24], 12, 100, false); // captured_len 100 > remaining 4 bytes
    expect_eq(parseEpb(epb_bad[0..24], false) == null, true);

    const classic_pcap: [12]u8 = [0xD4, 0xC3, 0xB2, 0xA1, 0, 0, 0, 0, 0, 0, 0, 0];
    expect_eq(isPcapng(classic_pcap[0..12]), false);
});
