// Source: cp/cap/test/test_smbios_production_deep.cp (Production deep tests for cap:smbios)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SMBIOS2_ANCHOR,
    SMBIOS2_DMI_ANCHOR,
    SMBIOS3_ANCHOR,
    SMBIOS2_EP_SIZE,
    SMBIOS3_EP_SIZE,
    SMBIOS_STRUCT_HEADER_SIZE,
    TYPE_BIOS,
    TYPE_SYSTEM,
    TYPE_BASEBOARD,
    TYPE_CHASSIS,
    TYPE_PROCESSOR,
    TYPE_MEMORY_DEVICE,
    TYPE_END_OF_TABLE,
    smbiosChecksum,
    Smbios2EntryPoint,
    parseSmbios2Entry,
    Smbios3EntryPoint,
    parseSmbios3Entry,
    SmbiosStruct,
    SmbiosTableIter,
    SmbiosBiosInfo,
    parseBiosInfo,
    SmbiosSystemInfo,
    parseSystemInfo,
    SmbiosMemoryDevice,
    parseMemoryDevice,
} from "cap:sys";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putU64Le(buf: []u8, off: usize, v: u64) {
    putU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    putU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

test("1. SMBIOS 2.1 dual-anchor dual-checksum Entry Point and SMBIOS 3.0 64-bit Entry Point", () => {
    expect_eq(SMBIOS2_EP_SIZE, 31 as usize);
    expect_eq(SMBIOS3_EP_SIZE, 24 as usize);
    expect_eq(SMBIOS_STRUCT_HEADER_SIZE, 4 as usize);

    // Build valid SMBIOS 2.1 Entry Point (31 bytes)
    let ep2: [31]u8 = std.mem.zeroes([31]u8);
    putBytes(ep2[0..], 0, SMBIOS2_ANCHOR);
    ep2[5] = 31;           // ep_len
    ep2[6] = 2;            // major
    ep2[7] = 8;            // minor
    putU16Le(ep2[0..], 8, 256);          // max_struct_size
    putBytes(ep2[0..], 16, SMBIOS2_DMI_ANCHOR);
    putU16Le(ep2[0..], 22, 4096);        // table_length
    putU32Le(ep2[0..], 24, 0x000F0000);  // table_addr
    putU16Le(ep2[0..], 28, 42);          // struct_count
    // Intermediate checksum at byte 21 over bytes 16..31
    ep2[21] = (256 - (smbiosChecksum(ep2[16..31]) as u32)) as u8;
    // Main checksum at byte 4 over bytes 0..31
    ep2[4] = (256 - (smbiosChecksum(ep2[0..31]) as u32)) as u8;

    const p2 = parseSmbios2Entry(ep2[0..]) ?? unreachable;
    expect_eq(p2.major, 2 as u8);
    expect_eq(p2.minor, 8 as u8);
    expect_eq(p2.max_struct_size, 256 as u16);
    expect_eq(p2.table_length, 4096 as u16);
    expect_eq(p2.table_addr, 0x000F0000 as u32);
    expect_eq(p2.struct_count, 42 as u16);

    // Build valid SMBIOS 3.0+ Entry Point (24 bytes)
    let ep3: [24]u8 = std.mem.zeroes([24]u8);
    putBytes(ep3[0..], 0, SMBIOS3_ANCHOR);
    ep3[6] = 24;           // ep_len
    ep3[7] = 3;            // major
    ep3[8] = 6;            // minor
    ep3[9] = 0;            // docrev
    putU32Le(ep3[0..], 12, 8192);
    putU64Le(ep3[0..], 16, 0x00000001_000E0000);
    ep3[5] = (256 - (smbiosChecksum(ep3[0..24]) as u32)) as u8;

    const p3 = parseSmbios3Entry(ep3[0..]) ?? unreachable;
    expect_eq(p3.major, 3 as u8);
    expect_eq(p3.minor, 6 as u8);
    expect_eq(p3.docrev, 0 as u8);
    expect_eq(p3.max_table_size, 8192 as u32);
    expect_eq(p3.table_addr, 0x00000001_000E0000 as u64);
});

test("2. SmbiosTableIter multi-structure traversal (Type 0 BIOS, Type 1 System, Type 127 End-of-Table)", () => {
    // Structure 1: Type 0 (BIOS), length=10, handle=0x0000
    //   formatted[4]=1 (vendor), formatted[5]=2 (version), formatted[8]=3 (date)
    //   strings: "CP-Firmware\0" "v2.4\0" "11/21/2026\0\0"
    // Structure 2: Type 1 (System), length=24, handle=0x0001
    //   formatted[4]=1, [5]=2, [6]=3, [7]=4, [8..24]=UUID
    //   strings: "CP Corp\0" "ServerX\0" "1.0\0" "SN-999\0\0"
    // Structure 3: Type 127 (End-of-Table), length=4, handle=0xFEFF, empty strings "\0\0"
    let table: [128]u8 = std.mem.zeroes([128]u8);
    let pos: usize = 0;

    // Type 0 (10B formatted)
    table[pos] = TYPE_BIOS;
    table[pos + 1] = 10;
    putU16Le(table[0..], pos + 2, 0x0000);
    table[pos + 4] = 1;
    table[pos + 5] = 2;
    table[pos + 8] = 3;
    pos += 10;
    putBytes(table[0..], pos, "CP-Firmware\x00v2.4\x0011/21/2026\x00\x00");
    pos += 29;

    // Type 1 (24B formatted)
    table[pos] = TYPE_SYSTEM;
    table[pos + 1] = 24;
    putU16Le(table[0..], pos + 2, 0x0001);
    table[pos + 4] = 1;
    table[pos + 5] = 2;
    table[pos + 6] = 3;
    table[pos + 7] = 4;
    table[pos + 8] = 0xAA;
    table[pos + 23] = 0xBB;
    pos += 24;
    putBytes(table[0..], pos, "CP Corp\x00ServerX\x001.0\x00SN-999\x00\x00");
    pos += 28;

    // Type 127 (4B formatted + empty string-set "\0\0")
    table[pos] = TYPE_END_OF_TABLE;
    table[pos + 1] = 4;
    putU16Le(table[0..], pos + 2, 0xFEFF);
    pos += 4;
    table[pos] = 0;
    table[pos + 1] = 0;
    pos += 2;

    let it = SmbiosTableIter.init(table[0..pos]);

    // BIOS
    const s0 = it.next() ?? unreachable;
    expect_eq(s0.struct_type, TYPE_BIOS);
    expect_eq(s0.stringAt(0) == null, true);
    expect_eq(s0.stringAt(99) == null, true);
    const bios = parseBiosInfo(s0) ?? unreachable;
    expect_eq(std.mem.eql(u8, bios.vendor, "CP-Firmware"), true);
    expect_eq(std.mem.eql(u8, bios.version, "v2.4"), true);
    expect_eq(std.mem.eql(u8, bios.release_date, "11/21/2026"), true);

    // System
    const s1 = it.next() ?? unreachable;
    expect_eq(s1.struct_type, TYPE_SYSTEM);
    expect_eq(s1.handle, 1 as u16);
    const sys = parseSystemInfo(s1) ?? unreachable;
    expect_eq(std.mem.eql(u8, sys.manufacturer, "CP Corp"), true);
    expect_eq(std.mem.eql(u8, sys.product_name, "ServerX"), true);
    expect_eq(std.mem.eql(u8, sys.version, "1.0"), true);
    expect_eq(std.mem.eql(u8, sys.serial_number, "SN-999"), true);
    expect_eq(sys.uuid[0], 0xAA as u8);
    expect_eq(sys.uuid[15], 0xBB as u8);

    // End of Table
    const s2 = it.next() ?? unreachable;
    expect_eq(s2.isEndOfTable(), true);
    expect_eq(s2.strings.len, 0 as usize);
    expect_eq(it.isDone(), true);
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("3. Type 17 Memory Device: standard 16GB DIMM and 0x7FFF extended 64GB DIMM", () => {
    // 1) Standard 16GB (16384 MB) DDR5-4800 DIMM
    let mem1: [48]u8 = std.mem.zeroes([48]u8);
    mem1[0] = TYPE_MEMORY_DEVICE;
    mem1[1] = 24;
    putU16Le(mem1[0..], 2, 0x0010);
    putU16Le(mem1[0..], 12, 16384); // 16384 MB
    mem1[16] = 1;                   // device_locator = "DIMM_A1"
    putU16Le(mem1[0..], 21, 4800);  // 4800 MT/s
    mem1[23] = 2;                   // manufacturer = "Micron"
    putBytes(mem1[0..], 24, "DIMM_A1\x00Micron\x00\x00");

    let it1 = SmbiosTableIter.init(mem1[0..40]);
    const st1 = it1.next() ?? unreachable;
    const d1 = parseMemoryDevice(st1) ?? unreachable;
    expect_eq(d1.size_mb, 16384 as u32);
    expect_eq(d1.speed_mts, 4800 as u16);
    expect_eq(std.mem.eql(u8, d1.device_locator, "DIMM_A1"), true);
    expect_eq(std.mem.eql(u8, d1.manufacturer, "Micron"), true);

    // 2) Extended 64GB (65536 MB) DIMM using 0x7FFF sentinel + offset 28 u32
    let mem2: [64]u8 = std.mem.zeroes([64]u8);
    mem2[0] = TYPE_MEMORY_DEVICE;
    mem2[1] = 32;
    putU16Le(mem2[0..], 12, 0x7FFF); // extended size indicator
    mem2[16] = 1;
    putU16Le(mem2[0..], 21, 5600);
    mem2[23] = 2;
    putU32Le(mem2[0..], 28, 65536);  // 64 GB in MB
    putBytes(mem2[0..], 32, "DIMM_B1\x00SK Hynix\x00\x00");

    let it2 = SmbiosTableIter.init(mem2[0..50]);
    const st2 = it2.next() ?? unreachable;
    const d2 = parseMemoryDevice(st2) ?? unreachable;
    expect_eq(d2.size_mb, 65536 as u32);
    expect_eq(d2.speed_mts, 5600 as u16);
    expect_eq(std.mem.eql(u8, d2.device_locator, "DIMM_B1"), true);
    expect_eq(std.mem.eql(u8, d2.manufacturer, "SK Hynix"), true);
});

test("4. Type 17 Memory Device: KB granularity bit and unpopulated (0x0000) slot", () => {
    // KB bit set (0x8000 | 4096 KB -> 4 MB)
    let mem_kb: [32]u8 = std.mem.zeroes([32]u8);
    mem_kb[0] = TYPE_MEMORY_DEVICE;
    mem_kb[1] = 21;
    putU16Le(mem_kb[0..], 12, 0x8000 | 4096);
    mem_kb[21] = 0;
    mem_kb[22] = 0;

    let it_kb = SmbiosTableIter.init(mem_kb[0..23]);
    const d_kb = parseMemoryDevice(it_kb.next() ?? unreachable) ?? unreachable;
    expect_eq(d_kb.size_mb, 4 as u32);

    // Unpopulated slot (raw_size = 0)
    putU16Le(mem_kb[0..], 12, 0);
    let it_empty = SmbiosTableIter.init(mem_kb[0..23]);
    const d_empty = parseMemoryDevice(it_empty.next() ?? unreachable) ?? unreachable;
    expect_eq(d_empty.size_mb, 0 as u32);
});

test("5. Malformed Entry Points, bad checksums, missing \\0\\0 terminator, and length < 4 rejection", () => {
    // Bad anchor for SMBIOS 2.1 & 3.0
    let bad2: [31]u8 = std.mem.zeroes([31]u8);
    putBytes(bad2[0..], 0, "_XX_");
    bad2[5] = 31;
    expect_eq(parseSmbios2Entry(bad2[0..]) == null, true);

    let bad3: [24]u8 = std.mem.zeroes([24]u8);
    putBytes(bad3[0..], 0, "_SM3_");
    bad3[6] = 24;
    bad3[5] = 0x42; // corrupted checksum
    expect_eq(parseSmbios3Entry(bad3[0..]) == null, true);

    // Truncated Entry Points
    expect_eq(parseSmbios2Entry(bad2[0..20]) == null, true);
    expect_eq(parseSmbios3Entry(bad3[0..16]) == null, true);

    // Structure with length < 4 rejected
    let bad_len: [8]u8 = [TYPE_BIOS, 3, 0, 0, 0, 0, 0, 0];
    let it_len = SmbiosTableIter.init(bad_len[0..]);
    expect_eq(it_len.next() == null, true);
    expect_eq(it_len.hasError(), true);

    // Missing "\0\0" double-NUL string terminator rejected
    let no_term: [10]u8 = [TYPE_BIOS, 4, 0, 0, 'A', 'B', 'C', 0, 'D', 'E'];
    let it_nt = SmbiosTableIter.init(no_term[0..]);
    expect_eq(it_nt.next() == null, true);
    expect_eq(it_nt.hasError(), true);

    // Wrong structure type passed to typed views returns null
    let eot: [6]u8 = [TYPE_END_OF_TABLE, 4, 0, 0, 0, 0];
    let it_eot = SmbiosTableIter.init(eot[0..]);
    const s_eot = it_eot.next() ?? unreachable;
    expect_eq(parseBiosInfo(s_eot) == null, true);
    expect_eq(parseSystemInfo(s_eot) == null, true);
    expect_eq(parseMemoryDevice(s_eot) == null, true);
});
