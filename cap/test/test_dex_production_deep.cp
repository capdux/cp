// Source: cp/cap/test/test_dex_production_deep.cp (Production deep tests for cap:dex)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DEX_HEADER_SIZE,
    DEX_ENDIAN_CONSTANT,
    DEX_REVERSE_ENDIAN_CONSTANT,
    DEX_TYPE_HEADER_ITEM,
    DEX_TYPE_STRING_ID_ITEM,
    DEX_TYPE_MAP_LIST,
    DexHeader,
    Uleb128Result,
    DexMapItem,
    DexMapIter,
    isDex,
    parseDexHeader,
    readUleb128,
    readDexString,
} from "cap:dex";

fn writeU16LE(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn initDexMagic(buf: []u8, v0: u8, v1: u8, v2: u8) {
    buf[0] = 'd'; buf[1] = 'e'; buf[2] = 'x'; buf[3] = '\n';
    buf[4] = v0; buf[5] = v1; buf[6] = v2; buf[7] = 0;
}

fn makeValidDexLE(buf: []u8, file_size: u32, v0: u8, v1: u8, v2: u8) {
    initDexMagic(buf, v0, v1, v2);
    writeU32LE(buf, 8, 0xCAFE_BABE); // checksum
    writeU32LE(buf, 32, file_size);
    writeU32LE(buf, 36, DEX_HEADER_SIZE);
    writeU32LE(buf, 40, DEX_ENDIAN_CONSTANT);
}

test("1. isDex and parseDexHeader decode standard DEX 035 and 039 little-endian headers", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    makeValidDexLE(buf[0..], 160, '0', '3', '5');
    writeU32LE(buf[0..], 104, 48); // data_size
    writeU32LE(buf[0..], 108, 112); // data_off

    expect_eq(isDex(buf[0..]), true);
    const h1 = parseDexHeader(buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, h1.version[0..], "035"), true);
    expect_eq(h1.checksum, 0xCAFE_BABE);
    expect_eq(h1.file_size, 160);
    expect_eq(h1.header_size, 112);
    expect_eq(h1.is_big_endian, false);
    expect_eq(h1.endian_tag, DEX_ENDIAN_CONSTANT);
    expect_eq(h1.data_size, 48);
    expect_eq(h1.data_off, 112);

    makeValidDexLE(buf[0..], 112, '0', '3', '9');
    const h2 = parseDexHeader(buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, h2.version[0..], "039"), true);
});

test("2. Big-endian DEX_REVERSE_ENDIAN_CONSTANT header parsing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    initDexMagic(buf[0..], '0', '4', '0');
    writeU32BE(buf[0..], 8, 0x1122_3344);
    writeU32BE(buf[0..], 32, 128);
    writeU32BE(buf[0..], 36, DEX_HEADER_SIZE);
    // Stored as LE 0x78563412 -> bytes 0x12 0x34 0x56 0x78, which reads as 0x12345678 in BE!
    writeU32LE(buf[0..], 40, DEX_REVERSE_ENDIAN_CONSTANT);
    writeU32BE(buf[0..], 104, 16);
    writeU32BE(buf[0..], 108, 112);

    const hdr = parseDexHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.is_big_endian, true);
    expect_eq(hdr.checksum, 0x1122_3344);
    expect_eq(hdr.file_size, 128);
    expect_eq(hdr.endian_tag, DEX_ENDIAN_CONSTANT);
    expect_eq(hdr.data_size, 16);
    expect_eq(hdr.data_off, 112);
});

test("3. readUleb128 1-5 byte boundary decoding and readDexString zero-copy lookup", () => {
    const u0: [1]u8 = [0x00];
    const r0 = readUleb128(u0[0..], 0) ?? unreachable;
    expect_eq(r0.value, 0);
    expect_eq(r0.bytes_read, 1);

    const u2: [2]u8 = [0x80, 0x01]; // 128
    const r2 = readUleb128(u2[0..], 0) ?? unreachable;
    expect_eq(r2.value, 128);
    expect_eq(r2.bytes_read, 2);

    // 5-byte max u32: 0xFFFF_FFFF -> 0xFF 0xFF 0xFF 0xFF 0x0F
    const umax: [5]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0x0F];
    const rmax = readUleb128(umax[0..], 0) ?? unreachable;
    expect_eq(rmax.value, 0xFFFF_FFFF);
    expect_eq(rmax.bytes_read, 5);

    // 5-byte overflow (5th byte > 0x0F) and truncated continuation rejected
    const uovf: [5]u8 = [0x80, 0x80, 0x80, 0x80, 0x10];
    expect_eq(readUleb128(uovf[0..], 0) == null, true);
    expect_eq(readUleb128(u2[0..1], 0) == null, true);

    // Construct DEX with 2 string_ids at offset 112 (8 bytes -> 112..120)
    // String 0 at offset 120: utf16_len=4, "main\0"
    // String 1 at offset 126: utf16_len=17, "Ljava/lang/Object;\0"
    let dex: [160]u8 = std.mem.zeroes([160]u8);
    makeValidDexLE(dex[0..], 160, '0', '3', '8');
    writeU32LE(dex[0..], 56, 2);   // string_ids_size = 2
    writeU32LE(dex[0..], 60, 112); // string_ids_off = 112
    writeU32LE(dex[0..], 112, 120);
    writeU32LE(dex[0..], 116, 126);

    dex[120] = 4;
    dex[121] = 'm'; dex[122] = 'a'; dex[123] = 'i'; dex[124] = 'n'; dex[125] = 0;

    dex[126] = 18;
    const cls = "Ljava/lang/Object;";
    let i: usize = 0;
    while (i < cls.len) {
        dex[127 + i] = cls[i];
        i += 1;
    }
    dex[127 + cls.len] = 0;

    const hdr = parseDexHeader(dex[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, readDexString(dex[0..], hdr, 0) ?? "", "main"), true);
    expect_eq(std.mem.eql(u8, readDexString(dex[0..], hdr, 1) ?? "", "Ljava/lang/Object;"), true);
    expect_eq(readDexString(dex[0..], hdr, 2) == null, true);
});

test("4. DexMapIter iterates map_off section table and guards against out-of-bounds count", () => {
    // 112B header + 4B count + 3*12B items = 152 bytes
    let dex: [152]u8 = std.mem.zeroes([152]u8);
    makeValidDexLE(dex[0..], 152, '0', '3', '5');
    writeU32LE(dex[0..], 52, 112); // map_off = 112

    writeU32LE(dex[0..], 112, 3); // count = 3
    // Item 0: HEADER_ITEM
    writeU16LE(dex[0..], 116, DEX_TYPE_HEADER_ITEM);
    writeU32LE(dex[0..], 120, 1);
    writeU32LE(dex[0..], 124, 0);
    // Item 1: STRING_ID_ITEM
    writeU16LE(dex[0..], 128, DEX_TYPE_STRING_ID_ITEM);
    writeU32LE(dex[0..], 132, 0);
    writeU32LE(dex[0..], 136, 0);
    // Item 2: MAP_LIST
    writeU16LE(dex[0..], 140, DEX_TYPE_MAP_LIST);
    writeU32LE(dex[0..], 144, 1);
    writeU32LE(dex[0..], 148, 112);

    const hdr = parseDexHeader(dex[0..]) ?? unreachable;
    let it = DexMapIter.init(dex[0..], hdr);

    const m0 = it.next() ?? unreachable;
    expect_eq(m0.item_type, DEX_TYPE_HEADER_ITEM);
    expect_eq(m0.size, 1);
    expect_eq(m0.offset, 0);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.item_type, DEX_TYPE_STRING_ID_ITEM);

    const m2 = it.next() ?? unreachable;
    expect_eq(m2.item_type, DEX_TYPE_MAP_LIST);
    expect_eq(m2.offset, 112);
    expect_eq(it.next() == null, true);

    // Inflated count exceeding buffer yields empty iterator safely
    writeU32LE(dex[0..], 112, 9999);
    let bad_it = DexMapIter.init(dex[0..], hdr);
    expect_eq(bad_it.next() == null, true);
});

test("5. Malformed magic, non-112 header_size, bad endian_tag, and overflowing section offsets fail safely", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    expect_eq(isDex(buf[0..7]), false);
    expect_eq(parseDexHeader(buf[0..111]) == null, true);

    // Non-digit version byte
    initDexMagic(buf[0..], '0', '3', 'X');
    expect_eq(isDex(buf[0..]), false);

    // Wrong header_size (100 != 112)
    makeValidDexLE(buf[0..], 128, '0', '3', '5');
    writeU32LE(buf[0..], 36, 100);
    expect_eq(parseDexHeader(buf[0..]) == null, true);

    // Wrong endian_tag
    makeValidDexLE(buf[0..], 128, '0', '3', '5');
    writeU32LE(buf[0..], 40, 0xDEAD_BEEF);
    expect_eq(parseDexHeader(buf[0..]) == null, true);

    // file_size > data.len
    makeValidDexLE(buf[0..], 512, '0', '3', '5');
    expect_eq(parseDexHeader(buf[0..]) == null, true);

    // Section offset + count*elem_size overflow (0xFFFF_FFF0 + 4*4)
    makeValidDexLE(buf[0..], 128, '0', '3', '5');
    writeU32LE(buf[0..], 56, 4);
    writeU32LE(buf[0..], 60, 0xFFFF_FFF0);
    expect_eq(parseDexHeader(buf[0..]) == null, true);
});
