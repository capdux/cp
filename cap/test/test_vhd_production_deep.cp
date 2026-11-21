// Source: cp/cap/test/test_vhd_production_deep.cp (Production deep tests for cap:vhd)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VHD_FOOTER_COOKIE,
    VHD_DYNAMIC_COOKIE,
    VHD_FOOTER_SIZE,
    VHD_DYNAMIC_HEADER_SIZE,
    VHD_FORMAT_VERSION,
    VHD_FIXED_DATA_OFFSET,
    VHD_EPOCH_UNIX_DELTA,
    VHD_BAT_UNUSED,
    DISK_TYPE_FIXED,
    DISK_TYPE_DYNAMIC,
    DISK_TYPE_DIFFERENCING,
    FEATURE_TEMPORARY,
    FEATURE_RESERVED,
    vhdChecksum,
    isVhdFooter,
    parseVhdFooter,
    parseVhdDynamicHeader,
    lookupVhdBatEntry,
} from "cap:vhd";

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

fn writeU64Be(buf: []u8, off: usize, v: u64) {
    writeU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn buildVhdFooter(
    buf: []u8,
    disk_type: u32,
    data_offset: u64,
    timestamp: u32,
    size_bytes: u64,
    saved_state: u8,
) {
    let i: usize = 0;
    while (i < VHD_FOOTER_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    copyInto(buf, 0, VHD_FOOTER_COOKIE);
    writeU32Be(buf, 8, FEATURE_RESERVED);
    writeU32Be(buf, 12, VHD_FORMAT_VERSION);
    writeU64Be(buf, 16, data_offset);
    writeU32Be(buf, 24, timestamp);
    copyInto(buf, 28, "cp  ");
    writeU32Be(buf, 32, 0x00010000);
    copyInto(buf, 36, "Wi2k");
    writeU64Be(buf, 40, size_bytes);
    writeU64Be(buf, 48, size_bytes);
    writeU16Be(buf, 56, 1024); // cylinders
    buf[58] = 16;              // heads
    buf[59] = 63;              // sectors_per_track
    writeU32Be(buf, 60, disk_type);
    let g: usize = 0;
    while (g < 16) {
        buf[68 + g] = (0xA0 + g) as u8;
        g += 1;
    }
    buf[84] = saved_state;

    const cksum = vhdChecksum(buf[0..VHD_FOOTER_SIZE], 64);
    writeU32Be(buf, 64, cksum);
}

fn buildVhdDynamicHeader(
    buf: []u8,
    table_offset: u64,
    max_entries: u32,
    block_size: u32,
) {
    let i: usize = 0;
    while (i < VHD_DYNAMIC_HEADER_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    copyInto(buf, 0, VHD_DYNAMIC_COOKIE);
    writeU64Be(buf, 8, VHD_FIXED_DATA_OFFSET);
    writeU64Be(buf, 16, table_offset);
    writeU32Be(buf, 24, VHD_FORMAT_VERSION);
    writeU32Be(buf, 28, max_entries);
    writeU32Be(buf, 32, block_size);
    let g: usize = 0;
    while (g < 16) {
        buf[40 + g] = (0x30 + g) as u8;
        g += 1;
    }
    writeU32Be(buf, 56, 123456);
    // Parent Unicode Name (UTF-16BE "base.vhd")
    buf[65] = 0x62; // 'b'
    buf[67] = 0x61; // 'a'

    const cksum = vhdChecksum(buf[0..VHD_DYNAMIC_HEADER_SIZE], 36);
    writeU32Be(buf, 36, cksum);
}

test("1. Fixed VHD 512B Footer parsing, one's-complement checksum, geometry, and 2000-01-01 epoch conversion", () => {
    expect_eq(VHD_FOOTER_SIZE, 512);
    expect_eq(VHD_DYNAMIC_HEADER_SIZE, 1024);
    expect_eq(VHD_FORMAT_VERSION, 0x00010000 as u32);
    expect_eq(VHD_EPOCH_UNIX_DELTA, 946684800 as u64);
    expect_eq(FEATURE_TEMPORARY, 0x01 as u32);
    expect_eq(FEATURE_RESERVED, 0x02 as u32);

    let footer_buf: [512]u8 = [0; 512];
    // 2020-01-01T00:00:00Z is 631152000 seconds after 2000-01-01T00:00:00Z -> Unix 1577836800
    buildVhdFooter(footer_buf[0..], DISK_TYPE_FIXED, VHD_FIXED_DATA_OFFSET, 631152000, 1073741824, 0);

    expect_eq(isVhdFooter(footer_buf[0..]), true);
    const f = parseVhdFooter(footer_buf[0..]) ?? unreachable;
    expect_eq(f.isFixed(), true);
    expect_eq(f.isDynamic(), false);
    expect_eq(f.isDifferencing(), false);
    expect_eq(f.features, FEATURE_RESERVED);
    expect_eq(f.format_version, VHD_FORMAT_VERSION);
    expect_eq(f.data_offset, VHD_FIXED_DATA_OFFSET);
    expect_eq(f.timestamp, 631152000 as u32);
    expect_eq(f.unixTimestamp(), 1577836800 as u64);
    expect_eq(std.mem.eql(u8, f.creator_app[0..], "cp  "), true);
    expect_eq(std.mem.eql(u8, f.creator_host_os[0..], "Wi2k"), true);
    expect_eq(f.original_size, 1073741824 as u64);
    expect_eq(f.current_size, 1073741824 as u64);
    expect_eq(f.geometry.cylinders, 1024 as u16);
    expect_eq(f.geometry.heads, 16 as u8);
    expect_eq(f.geometry.sectors_per_track, 63 as u8);
    expect_eq(f.unique_id[0], 0xA0 as u8);
    expect_eq(f.saved_state, false);
});

test("2. Dynamic and Differencing VHD 512B Footers with saved_state flag", () => {
    let dyn_buf: [512]u8 = [0; 512];
    buildVhdFooter(dyn_buf[0..], DISK_TYPE_DYNAMIC, 512, 0, 2147483648, 1);
    const f_dyn = parseVhdFooter(dyn_buf[0..]) ?? unreachable;
    expect_eq(f_dyn.isDynamic(), true);
    expect_eq(f_dyn.isFixed(), false);
    expect_eq(f_dyn.data_offset, 512 as u64);
    expect_eq(f_dyn.unixTimestamp(), VHD_EPOCH_UNIX_DELTA);
    expect_eq(f_dyn.saved_state, true);

    let diff_buf: [512]u8 = [0; 512];
    buildVhdFooter(diff_buf[0..], DISK_TYPE_DIFFERENCING, 512, 86400, 2147483648, 0);
    const f_diff = parseVhdFooter(diff_buf[0..]) ?? unreachable;
    expect_eq(f_diff.isDifferencing(), true);
    expect_eq(f_diff.unixTimestamp(), VHD_EPOCH_UNIX_DELTA + 86400);
    expect_eq(f_diff.saved_state, false);
});

test("3. 1024B VHD Dynamic Disk Header parsing, checksum verification, and block/BAT geometry", () => {
    let hdr_buf: [1024]u8 = [0; 1024];
    // 2MB blocks (0x00200000), 1024 entries, BAT at offset 1536
    buildVhdDynamicHeader(hdr_buf[0..], 1536, 1024, 2097152);

    const dyn_hdr = parseVhdDynamicHeader(hdr_buf[0..]) ?? unreachable;
    expect_eq(dyn_hdr.data_offset, VHD_FIXED_DATA_OFFSET);
    expect_eq(dyn_hdr.table_offset, 1536 as u64);
    expect_eq(dyn_hdr.header_version, VHD_FORMAT_VERSION);
    expect_eq(dyn_hdr.max_table_entries, 1024 as u32);
    expect_eq(dyn_hdr.block_size, 2097152 as u32);
    expect_eq(dyn_hdr.blockSectorCount(), 4096 as u32);
    expect_eq(dyn_hdr.batSizeBytes(), 4096 as u64);
    expect_eq(dyn_hdr.parent_unique_id[0], 0x30 as u8);
    expect_eq(dyn_hdr.parent_timestamp, 123456 as u32);
    expect_eq(dyn_hdr.parent_unicode_name[1], 0x62 as u8);
    expect_eq(dyn_hdr.parent_unicode_name[3], 0x61 as u8);
});

test("4. Big-endian BAT sector lookup and 0xFFFFFFFF unused block handling", () => {
    let bat: [16]u8 = [0; 16]; // 4 entries
    writeU32Be(bat[0..], 0, 12);             // block 0 -> sector 12
    writeU32Be(bat[0..], 4, VHD_BAT_UNUSED); // block 1 -> unallocated
    writeU32Be(bat[0..], 8, 4120);           // block 2 -> sector 4120
    writeU32Be(bat[0..], 12, 0);             // block 3 -> sector 0

    expect_eq(lookupVhdBatEntry(bat[0..], 0) ?? 9999, 12 as u32);
    expect_eq(lookupVhdBatEntry(bat[0..], 1) == null, true);
    expect_eq(lookupVhdBatEntry(bat[0..], 2) ?? 9999, 4120 as u32);
    expect_eq(lookupVhdBatEntry(bat[0..], 3) ?? 9999, 0 as u32);

    // Out-of-bounds block index
    expect_eq(lookupVhdBatEntry(bat[0..], 4) == null, true);
    expect_eq(lookupVhdBatEntry(bat[0..], 100) == null, true);
});

test("5. Malformed VHD Footer & Dynamic Header rejection: bad cookies, bad checksum, invalid disk_type/block_size, truncation", () => {
    // Truncated footer & bad cookie
    expect_eq(isVhdFooter("notavhd!"), false);
    expect_eq(parseVhdFooter("conectix") == null, true);

    let fbuf: [512]u8 = [0; 512];

    // Corrupted footer checksum
    buildVhdFooter(fbuf[0..], DISK_TYPE_FIXED, VHD_FIXED_DATA_OFFSET, 100, 1048576, 0);
    fbuf[100] ^= 0x01;
    expect_eq(parseVhdFooter(fbuf[0..]) == null, true);

    // Invalid format_version (recomputed checksum still rejected)
    buildVhdFooter(fbuf[0..], DISK_TYPE_FIXED, VHD_FIXED_DATA_OFFSET, 100, 1048576, 0);
    writeU32Be(fbuf[0..], 12, 0x00020000);
    writeU32Be(fbuf[0..], 64, vhdChecksum(fbuf[0..], 64));
    expect_eq(parseVhdFooter(fbuf[0..]) == null, true);

    // Invalid disk_type (0, 1, 5)
    buildVhdFooter(fbuf[0..], 1, VHD_FIXED_DATA_OFFSET, 100, 1048576, 0);
    expect_eq(parseVhdFooter(fbuf[0..]) == null, true);
    buildVhdFooter(fbuf[0..], 5, VHD_FIXED_DATA_OFFSET, 100, 1048576, 0);
    expect_eq(parseVhdFooter(fbuf[0..]) == null, true);

    // Dynamic Header: truncated
    let short_dyn: [128]u8 = [0; 128];
    copyInto(short_dyn[0..], 0, VHD_DYNAMIC_COOKIE);
    expect_eq(parseVhdDynamicHeader(short_dyn[0..]) == null, true);

    let dbuf: [1024]u8 = [0; 1024];

    // Dynamic Header: bad cookie
    buildVhdDynamicHeader(dbuf[0..], 1536, 16, 2097152);
    dbuf[0] = 0x58;
    expect_eq(parseVhdDynamicHeader(dbuf[0..]) == null, true);

    // Dynamic Header: bad checksum
    buildVhdDynamicHeader(dbuf[0..], 1536, 16, 2097152);
    dbuf[200] ^= 0x80;
    expect_eq(parseVhdDynamicHeader(dbuf[0..]) == null, true);

    // Dynamic Header: block_size < 512 or non-power-of-two (even with valid checksum)
    buildVhdDynamicHeader(dbuf[0..], 1536, 16, 256);
    expect_eq(parseVhdDynamicHeader(dbuf[0..]) == null, true);
    buildVhdDynamicHeader(dbuf[0..], 1536, 16, 1536);
    expect_eq(parseVhdDynamicHeader(dbuf[0..]) == null, true);
});
