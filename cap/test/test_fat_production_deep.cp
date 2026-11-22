// Source: cp/cap/test/test_fat_production_deep.cp (Production deep tests for cap:fat)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FAT_BOOT_SECTOR_SIZE,
    FAT_BOOT_SIGNATURE,
    FAT_DIR_ENTRY_SIZE,
    ATTR_READ_ONLY,
    ATTR_HIDDEN,
    ATTR_SYSTEM,
    ATTR_VOLUME_ID,
    ATTR_DIRECTORY,
    ATTR_ARCHIVE,
    ATTR_LFN,
    FatKind,
    FatDateTime,
    decodeDosDateTime,
    fatShortNameChecksum,
    FatBpb,
    isFatBootSector,
    parseFatBpb,
    FatDirEntry,
    FatDirIter,
} from "cap:sys";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn writeU16LeTest(buf: []u8, off: usize, val: u16) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32LeTest(buf: []u8, off: usize, val: u32) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = ((val >> 16) & 0xFF) as u8;
    buf[off + 3] = ((val >> 24) & 0xFF) as u8;
}

test("1. FAT12 (1.44MB floppy), FAT16, and FAT32 BPB classification & LBA calculation", () => {
    expect_eq(FAT_BOOT_SECTOR_SIZE, 512);
    expect_eq(FAT_BOOT_SIGNATURE, 0xAA55 as u16);
    expect_eq(FAT_DIR_ENTRY_SIZE, 32);

    // 1) Classic 1.44MB FAT12 floppy BPB: 2880 sectors, 512 B/s, 1 s/c, 1 res, 2 FATs * 9, 224 root entries
    // root_dir_sectors = 14, data_start = 1 + 18 + 14 = 33, cluster_count = 2847 (< 4085 => FAT12)
    let f12: [512]u8 = [0; 512];
    writeU16LeTest(f12[0..], 11, 512);
    f12[13] = 1;
    writeU16LeTest(f12[0..], 14, 1);
    f12[16] = 2;
    writeU16LeTest(f12[0..], 17, 224);
    writeU16LeTest(f12[0..], 19, 2880);
    writeU16LeTest(f12[0..], 22, 9);
    writeU16LeTest(f12[0..], 510, FAT_BOOT_SIGNATURE);

    expect_eq(isFatBootSector(f12[0..]), true);
    const b12 = parseFatBpb(f12[0..]) ?? unreachable;
    expect_eq(b12.kind == FatKind.fat12, true);
    expect_eq(b12.fatStartLba(), 1 as u32);
    expect_eq(b12.rootDirStartLba(), 19 as u32);
    expect_eq(b12.dataStartLba(), 33 as u32);
    expect_eq(b12.cluster_count, 2847 as u32);
    expect_eq(b12.clusterToLba(2) ?? 0, 33 as u32);
    expect_eq(b12.clusterToLba(5) ?? 0, 36 as u32);
    expect_eq(b12.clusterToLba(1) == null, true);

    // 2) FAT16: 65536 sectors, 512 B/s, 4 s/c, 1 res, 2 FATs * 64, 512 root entries (32 sec)
    // data_start = 1 + 128 + 32 = 161, data_sec = 65375, clusters = 16343 (4085 <= c < 65525 => FAT16)
    let f16: [512]u8 = [0; 512];
    writeU16LeTest(f16[0..], 11, 512);
    f16[13] = 4;
    writeU16LeTest(f16[0..], 14, 1);
    f16[16] = 2;
    writeU16LeTest(f16[0..], 17, 512);
    writeU32LeTest(f16[0..], 32, 65536);
    writeU16LeTest(f16[0..], 22, 64);
    writeU16LeTest(f16[0..], 510, FAT_BOOT_SIGNATURE);

    const b16 = parseFatBpb(f16[0..]) ?? unreachable;
    expect_eq(b16.kind == FatKind.fat16, true);
    expect_eq(b16.dataStartLba(), 161 as u32);
    expect_eq(b16.clusterSizeBytes(), 2048 as u32);
    expect_eq(b16.clusterToLba(3) ?? 0, 165 as u32);

    // 3) FAT32: 1048576 sectors (512MB), 512 B/s, 8 s/c, 32 res, 2 FATs * 1024, root_cluster=2
    // data_start = 32 + 2048 = 2080, clusters = 130812 (>= 65525 => FAT32)
    let f32: [512]u8 = [0; 512];
    writeU16LeTest(f32[0..], 11, 512);
    f32[13] = 8;
    writeU16LeTest(f32[0..], 14, 32);
    f32[16] = 2;
    writeU32LeTest(f32[0..], 32, 1048576);
    writeU32LeTest(f32[0..], 36, 1024);
    writeU32LeTest(f32[0..], 44, 2);
    writeU32LeTest(f32[0..], 67, 0x12345678);
    writeU16LeTest(f32[0..], 510, FAT_BOOT_SIGNATURE);

    const b32 = parseFatBpb(f32[0..]) ?? unreachable;
    expect_eq(b32.kind == FatKind.fat32, true);
    expect_eq(b32.dataStartLba(), 2080 as u32);
    expect_eq(b32.rootDirStartLba(), 2080 as u32);
    expect_eq(b32.volume_id, 0x12345678 as u32);
    expect_eq(b32.clusterSizeBytes(), 4096 as u32);
});

test("2. 32B FatDirEntry 8.3 short name formatting & 32-bit cluster assembly", () => {
    let dir: [64]u8 = [0; 64];
    // Entry 0: "README  TXT", attr=ARCHIVE|READ_ONLY, clus_hi=0x0002, clus_lo=0x0015 => 0x00020015, size=1024
    const n0 = "README  TXT";
    let i: usize = 0;
    while (i < 11) {
        dir[i] = n0[i];
        i += 1;
    }
    dir[11] = ATTR_ARCHIVE | ATTR_READ_ONLY;
    writeU16LeTest(dir[0..], 20, 0x0002);
    writeU16LeTest(dir[0..], 26, 0x0015);
    writeU32LeTest(dir[0..], 28, 1024);

    // Entry 1: "DOCS       " (directory without extension)
    const n1 = "DOCS       ";
    let j: usize = 0;
    while (j < 11) {
        dir[32 + j] = n1[j];
        j += 1;
    }
    dir[32 + 11] = ATTR_DIRECTORY | ATTR_HIDDEN;
    writeU16LeTest(dir[0..], 32 + 26, 0x0003);

    let it = FatDirIter.init(dir[0..], true);
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.first_cluster, 0x00020015 as u32);
    expect_eq(e0.file_size, 1024 as u32);
    expect_eq(e0.isReadOnly(), true);
    expect_eq(e0.isDirectory(), false);

    let name_buf: [16]u8 = [0; 16];
    const l0 = e0.formatShortName(name_buf[0..]);
    expectBytesEq(name_buf[0..l0], "README.TXT");

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.isDirectory(), true);
    expect_eq(e1.isHidden(), true);
    const l1 = e1.formatShortName(name_buf[0..]);
    expectBytesEq(name_buf[0..l1], "DOCS");
    expect_eq(it.next() == null, true);
});

test("3. DOS date/time decoding and Microsoft LFN 8-bit short name checksum", () => {
    // 2026-11-22 14:35:24 -> year_off=46 (0x2E), month=11, day=22 => (46<<9)|(11<<5)|22 = 0x5D76
    // hour=14, min=35, sec/2=12 => (14<<11)|(35<<5)|12 = 0x746C
    const dt = decodeDosDateTime(0x5D76, 0x746C);
    expect_eq(dt.year, 2026 as u16);
    expect_eq(dt.month, 11 as u8);
    expect_eq(dt.day, 22 as u8);
    expect_eq(dt.hour, 14 as u8);
    expect_eq(dt.minute, 35 as u8);
    expect_eq(dt.second, 24 as u8);

    // LFN checksum determinism & sensitivity check
    const s1: [11]u8 = ['R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'T'];
    const s2: [11]u8 = ['R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'U'];
    const c1 = fatShortNameChecksum(s1);
    const c2 = fatShortNameChecksum(s2);
    expect_eq(c1 == c2, false);
    // Known manual vector for "           " (11 spaces, 0x20):
    const spaces: [11]u8 = [' '; 11];
    expect_eq(fatShortNameChecksum(spaces), 0xDC as u8);
});

test("4. FatDirIter skips 0xE5 deleted entries, filters/yields LFN, and stops at 0x00", () => {
    let buf: [160]u8 = [0; 160];
    // Slot 0: deleted (0xE5)
    buf[0] = 0xE5;
    // Slot 1: LFN entry (attr = 0x0F)
    buf[32] = 0x41;
    buf[32 + 11] = ATTR_LFN;
    // Slot 2: valid file "APP     BIN"
    const fname = "APP     BIN";
    let i: usize = 0;
    while (i < 11) {
        buf[64 + i] = fname[i];
        i += 1;
    }
    buf[64 + 11] = ATTR_ARCHIVE;
    // Slot 3: end-of-directory sentinel (0x00)
    buf[96] = 0x00;
    // Slot 4: unreachable entry after 0x00
    buf[128] = 'Z';
    buf[128 + 11] = ATTR_ARCHIVE;

    // With skip_lfn = true: yields only "APP.BIN"
    let it_skip = FatDirIter.init(buf[0..], true);
    const f = it_skip.next() ?? unreachable;
    let out: [16]u8 = [0; 16];
    const n = f.formatShortName(out[0..]);
    expectBytesEq(out[0..n], "APP.BIN");
    expect_eq(it_skip.next() == null, true);

    // With skip_lfn = false: yields LFN entry first, then "APP.BIN"
    let it_all = FatDirIter.init(buf[0..], false);
    const lfn = it_all.next() ?? unreachable;
    expect_eq(lfn.isLfn(), true);
    expect_eq(lfn.isVolumeId(), false);
    const real = it_all.next() ?? unreachable;
    expect_eq(real.isLfn(), false);
    expect_eq(it_all.next() == null, true);
});

test("5. Malformed Boot Sector BPB rejection (bad signature, non-pow2 spc, bad bps, zero FAT)", () => {
    let base: [512]u8 = [0; 512];
    writeU16LeTest(base[0..], 11, 512);
    base[13] = 1;
    writeU16LeTest(base[0..], 14, 1);
    base[16] = 2;
    writeU16LeTest(base[0..], 17, 224);
    writeU16LeTest(base[0..], 19, 2880);
    writeU16LeTest(base[0..], 22, 9);
    writeU16LeTest(base[0..], 510, FAT_BOOT_SIGNATURE);
    expect_eq(isFatBootSector(base[0..]), true);

    // 1. Short sector (< 512B)
    expect_eq(parseFatBpb(base[0..511]) == null, true);

    // 2. Bad 0xAA55 signature
    let bad_sig: [512]u8 = base;
    bad_sig[511] = 0x55;
    expect_eq(parseFatBpb(bad_sig[0..]) == null, true);

    // 3. Invalid bytes_per_sector (300)
    let bad_bps: [512]u8 = base;
    writeU16LeTest(bad_bps[0..], 11, 300);
    expect_eq(parseFatBpb(bad_bps[0..]) == null, true);

    // 4. Non-power-of-two sectors_per_cluster (3) and zero (0)
    let bad_spc: [512]u8 = base;
    bad_spc[13] = 3;
    expect_eq(parseFatBpb(bad_spc[0..]) == null, true);
    bad_spc[13] = 0;
    expect_eq(parseFatBpb(bad_spc[0..]) == null, true);

    // 5. num_fats == 0 or fat_size == 0
    let bad_fats: [512]u8 = base;
    bad_fats[16] = 0;
    expect_eq(parseFatBpb(bad_fats[0..]) == null, true);
    let bad_fsz: [512]u8 = base;
    writeU16LeTest(bad_fsz[0..], 22, 0);
    expect_eq(parseFatBpb(bad_fsz[0..]) == null, true);
});
