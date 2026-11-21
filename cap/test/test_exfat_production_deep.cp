// Source: cp/cap/test/test_exfat_production_deep.cp (Production deep tests for cap:exfat)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EXFAT_OEM_ID,
    EXFAT_BOOT_SIGNATURE,
    EXFAT_MIN_BOOT_SIZE,
    EXFAT_DIR_ENTRY_SIZE,
    ENTRY_EOD,
    ENTRY_ALLOC_BITMAP,
    ENTRY_UPCASE_TABLE,
    ENTRY_VOLUME_LABEL,
    ENTRY_FILE,
    ENTRY_STREAM_EXT,
    ENTRY_FILE_NAME,
    ATTR_READ_ONLY,
    ATTR_HIDDEN,
    ATTR_SYSTEM,
    ATTR_DIRECTORY,
    ATTR_ARCHIVE,
    STREAM_ALLOC_POSSIBLE,
    STREAM_NO_FAT_CHAIN,
    VOLUME_FLAG_DIRTY,
    ExfatBootSector,
    isExfatBootSector,
    parseExfatBootSector,
    exfatBootChecksum,
    exfatEntrySetChecksum,
    ExfatFileEntry,
    parseFileEntry,
    ExfatStreamExt,
    parseStreamExt,
    decodeFileNameAscii,
    ExfatRawEntry,
    ExfatDirIter,
} from "cap:exfat";

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

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn initExfatBoot(buf: []u8, bps_shift: u8, spc_shift: u8, num_fats: u8, volume_flags: u16) {
    buf[0] = 0xEB;
    buf[1] = 0x76;
    buf[2] = 0x90;
    copyBytes(buf, 3, EXFAT_OEM_ID);
    putU64Le(buf, 64, 2048); // partition_offset
    putU64Le(buf, 72, 2097152); // volume_length (1 GiB @ 512B)
    putU32Le(buf, 80, 24); // fat_offset
    putU32Le(buf, 84, 1024); // fat_length
    putU32Le(buf, 88, 2048); // cluster_heap_offset
    putU32Le(buf, 92, 32704); // cluster_count
    putU32Le(buf, 96, 4); // first_cluster_of_root
    putU32Le(buf, 100, 0xCAFEBABE); // volume_serial
    putU16Le(buf, 104, 0x0100); // fs_revision 1.00
    putU16Le(buf, 106, volume_flags);
    buf[108] = bps_shift;
    buf[109] = spc_shift;
    buf[110] = num_fats;
    buf[111] = 0x80; // drive_select
    buf[112] = 35; // percent_in_use
    putU16Le(buf, 510, EXFAT_BOOT_SIGNATURE);
}

fn writeUtf16LeName(entry: []u8, name: []const u8) {
    entry[0] = ENTRY_FILE_NAME;
    entry[1] = 0;
    let i: usize = 0;
    while (i < name.len && i < 15) {
        putU16Le(entry, 2 + i * 2, name[i] as u16);
        i += 1;
    }
}

test("1. exFAT 512B Boot Sector parsing, cluster geometry & clusterToLba", () => {
    expect_eq(EXFAT_MIN_BOOT_SIZE, 512 as usize);
    expect_eq(EXFAT_DIR_ENTRY_SIZE, 32 as usize);
    expect_eq(EXFAT_BOOT_SIGNATURE, 0xAA55 as u16);
    expect_eq(ENTRY_ALLOC_BITMAP, 0x81 as u8);
    expect_eq(ENTRY_UPCASE_TABLE, 0x82 as u8);
    expect_eq(ATTR_READ_ONLY, 0x0001 as u16);
    expect_eq(ATTR_HIDDEN, 0x0002 as u16);
    expect_eq(ATTR_SYSTEM, 0x0004 as u16);
    expect_eq(ATTR_ARCHIVE, 0x0020 as u16);

    let sec0: [512]u8 = std.mem.zeroes([512]u8);
    // 512B sector (shift=9), 64 sectors/cluster (shift=6) -> 32 KiB clusters
    initExfatBoot(sec0[0..], 9, 6, 1, VOLUME_FLAG_DIRTY);

    expect_eq(isExfatBootSector(sec0[0..]), true);
    const bs = parseExfatBootSector(sec0[0..]) ?? unreachable;

    expect_eq(bs.partition_offset, 2048 as u64);
    expect_eq(bs.volume_length, 2097152 as u64);
    expect_eq(bs.fat_offset, 24 as u32);
    expect_eq(bs.fat_length, 1024 as u32);
    expect_eq(bs.cluster_heap_offset, 2048 as u32);
    expect_eq(bs.cluster_count, 32704 as u32);
    expect_eq(bs.first_cluster_of_root, 4 as u32);
    expect_eq(bs.volume_serial, 0xCAFEBABE as u32);
    expect_eq(bs.fs_revision, 0x0100 as u16);
    expect_eq(bs.number_of_fats, 1 as u8);
    expect_eq(bs.percent_in_use, 35 as u8);
    expect_eq(bs.isDirty(), true);

    expect_eq(bs.bytesPerSector(), 512 as u32);
    expect_eq(bs.sectorsPerCluster(), 64 as u32);
    expect_eq(bs.clusterSizeBytes(), 32768 as u32);

    // Cluster 2 -> cluster_heap_offset (2048)
    const lba2 = bs.clusterToLba(2) ?? unreachable;
    expect_eq(lba2, 2048 as u64);
    // Root cluster 4 -> 2048 + (4 - 2) * 64 = 2176
    const lba4 = bs.clusterToLba(4) ?? unreachable;
    expect_eq(lba4, 2176 as u64);
    // Out-of-range clusters rejected
    expect_eq(bs.clusterToLba(0) == null, true);
    expect_eq(bs.clusterToLba(1) == null, true);
    expect_eq(bs.clusterToLba(32706) == null, true);
});

test("2. exFAT 11-sector Boot Region 32-bit checksum ignores volatile offsets 106, 107, 112", () => {
    let region: [5632]u8 = std.mem.zeroes([5632]u8); // 11 * 512 bytes
    initExfatBoot(region[0..512], 9, 6, 1, 0x0000);
    // Extended boot signature on sectors 1..8
    let s: usize = 1;
    while (s <= 8) {
        putU16Le(region[0..], s * 512 + 510, EXFAT_BOOT_SIGNATURE);
        s += 1;
    }

    const cs1 = exfatBootChecksum(region[0..]);
    expect_eq(cs1 != 0, true);

    // Mutate VolumeFlags (106, 107) and PercentInUse (112): checksum MUST remain identical
    region[106] = 0xFF;
    region[107] = 0xAA;
    region[112] = 99;
    const cs2 = exfatBootChecksum(region[0..]);
    expect_eq(cs1, cs2);

    // Mutate a non-volatile byte (e.g. volume_serial at 100): checksum MUST change
    region[100] = region[100] ^ 0x01;
    const cs3 = exfatBootChecksum(region[0..]);
    expect_eq(cs3 != cs1, true);
});

test("3. File + StreamExt + FileName complete directory entry set parsing & 16-bit SetChecksum verification", () => {
    let set_bytes: [96]u8 = std.mem.zeroes([96]u8); // 3 * 32B: File + StreamExt + FileName

    // Entry 0: File Entry (0x85)
    set_bytes[0] = ENTRY_FILE;
    set_bytes[1] = 2; // secondary_count = 2 (StreamExt + 1 FileName)
    putU16Le(set_bytes[0..], 4, ATTR_ARCHIVE | ATTR_READ_ONLY);
    putU32Le(set_bytes[0..], 8, 0x5821A000); // create_ts
    putU32Le(set_bytes[0..], 12, 0x5821B000); // last_mod_ts
    putU32Le(set_bytes[0..], 16, 0x5821C000); // last_acc_ts

    // Entry 1: Stream Extension (0xC0)
    set_bytes[32] = ENTRY_STREAM_EXT;
    set_bytes[33] = STREAM_ALLOC_POSSIBLE | STREAM_NO_FAT_CHAIN;
    set_bytes[35] = 10; // name_length = 10 ("README.TXT")
    putU16Le(set_bytes[0..], 36, 0x1234); // name_hash
    putU64Le(set_bytes[0..], 40, 4096); // valid_data_length
    putU32Le(set_bytes[0..], 52, 12); // first_cluster
    putU64Le(set_bytes[0..], 56, 4096); // data_length

    // Entry 2: File Name (0xC1)
    writeUtf16LeName(set_bytes[64..96], "README.TXT");

    // Compute and store SetChecksum at bytes 2..3
    const expected_cs = exfatEntrySetChecksum(set_bytes[0..]);
    putU16Le(set_bytes[0..], 2, expected_cs);

    // Recompute checksum with SetChecksum field populated: MUST match because offsets 2, 3 are skipped
    expect_eq(exfatEntrySetChecksum(set_bytes[0..]), expected_cs);

    const f_ent = parseFileEntry(set_bytes[0..32]) ?? unreachable;
    expect_eq(f_ent.secondary_count, 2 as u8);
    expect_eq(f_ent.set_checksum, expected_cs);
    expect_eq(f_ent.file_attributes, ATTR_ARCHIVE | ATTR_READ_ONLY);
    expect_eq(f_ent.isDirectory(), false);
    expect_eq(f_ent.create_ts, 0x5821A000 as u32);
    expect_eq(f_ent.last_mod_ts, 0x5821B000 as u32);
    expect_eq(f_ent.last_acc_ts, 0x5821C000 as u32);

    const s_ent = parseStreamExt(set_bytes[32..64]) ?? unreachable;
    expect_eq(s_ent.isAllocationPossible(), true);
    expect_eq(s_ent.isContiguous(), true);
    expect_eq(s_ent.name_length, 10 as u8);
    expect_eq(s_ent.name_hash, 0x1234 as u16);
    expect_eq(s_ent.valid_data_length, 4096 as u64);
    expect_eq(s_ent.first_cluster, 12 as u32);
    expect_eq(s_ent.data_length, 4096 as u64);

    let name_buf: [15]u8 = [0; 15];
    const n = decodeFileNameAscii(set_bytes[64..96], name_buf[0..]);
    expect_eq(n, 10 as usize);
    expect_eq(eqStr(name_buf[0..n], "README.TXT"), true);
});

test("4. ExfatDirIter skips deleted entries, stops on EOD (0x00), and handles directory + non-ASCII fallback", () => {
    let dir: [192]u8 = std.mem.zeroes([192]u8); // 6 slots * 32B

    // Slot 0: Volume Label (0x83)
    dir[0] = ENTRY_VOLUME_LABEL;
    dir[1] = 4;

    // Slot 1: Deleted File Entry (0x05 -> InUse bit 7 == 0) -> MUST be skipped
    dir[32] = 0x05;
    dir[33] = 2;

    // Slot 2: Active Directory File Entry (0x85)
    dir[64] = ENTRY_FILE;
    dir[65] = 2;
    putU16Le(dir[0..], 68, ATTR_DIRECTORY);

    // Slot 3: StreamExt (0xC0) without NoFatChain (fragmented)
    dir[96] = ENTRY_STREAM_EXT;
    dir[97] = STREAM_ALLOC_POSSIBLE;
    dir[99] = 4;
    putU32Le(dir[0..], 116, 7);

    // Slot 4: FileName (0xC1) with a non-ASCII UTF-16LE code unit (0x4E2D) -> mapped to '?'
    dir[128] = ENTRY_FILE_NAME;
    putU16Le(dir[0..], 130, 0x0064); // 'd'
    putU16Le(dir[0..], 132, 0x0069); // 'i'
    putU16Le(dir[0..], 134, 0x0072); // 'r'
    putU16Le(dir[0..], 136, 0x4E2D); // non-ASCII -> '?'

    // Slot 5: ENTRY_EOD (0x00) -> iteration stops here
    dir[160] = ENTRY_EOD;

    let it = ExfatDirIter.init(dir[0..]);

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.isInUse(), true);
    expect_eq(r0.isVolumeLabel(), true);
    expect_eq(r0.isFile(), false);

    // Deleted slot 1 was skipped; next is slot 2 (File)
    const r1 = it.next() ?? unreachable;
    expect_eq(r1.isFile(), true);
    const dir_file = parseFileEntry(r1.raw[0..]) ?? unreachable;
    expect_eq(dir_file.isDirectory(), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.isStreamExt(), true);
    const st = parseStreamExt(r2.raw[0..]) ?? unreachable;
    expect_eq(st.isContiguous(), false);
    expect_eq(st.first_cluster, 7 as u32);

    const r3 = it.next() ?? unreachable;
    expect_eq(r3.isFileName(), true);
    let out: [15]u8 = [0; 15];
    const len = decodeFileNameAscii(r3.raw[0..], out[0..]);
    expect_eq(len, 4 as usize);
    expect_eq(eqStr(out[0..len], "dir?"), true);

    // EOD reached
    expect_eq(it.next() == null, true);
    expect_eq(it.next() == null, true);
});

test("5. Malformed Boot Sector rejection (bad OEM ID, bad signature, shift out-of-range, number_of_fats, truncated)", () => {
    // 1. Truncated buffer
    let short_buf: [256]u8 = std.mem.zeroes([256]u8);
    expect_eq(isExfatBootSector(short_buf[0..]), false);
    expect_eq(parseExfatBootSector(short_buf[0..]) == null, true);

    // 2. Bad OEM ID
    let bad_oem: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_oem[0..], 9, 6, 1, 0);
    copyBytes(bad_oem[0..], 3, "NTFS    ");
    expect_eq(isExfatBootSector(bad_oem[0..]), false);
    expect_eq(parseExfatBootSector(bad_oem[0..]) == null, true);

    // 3. Bad boot signature
    let bad_sig: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_sig[0..], 9, 6, 1, 0);
    putU16Le(bad_sig[0..], 510, 0x0000);
    expect_eq(isExfatBootSector(bad_sig[0..]), false);
    expect_eq(parseExfatBootSector(bad_sig[0..]) == null, true);

    // 4. bytes_per_sector_shift out of 9..12
    let bad_bps: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_bps[0..], 8, 4, 1, 0);
    expect_eq(parseExfatBootSector(bad_bps[0..]) == null, true);
    bad_bps[108] = 13;
    expect_eq(parseExfatBootSector(bad_bps[0..]) == null, true);

    // 5. bps_shift + spc_shift > 25
    let bad_spc: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_spc[0..], 12, 14, 1, 0); // 12 + 14 = 26 > 25
    expect_eq(parseExfatBootSector(bad_spc[0..]) == null, true);

    // 6. number_of_fats == 0 or > 2
    let bad_fats: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_fats[0..], 9, 6, 0, 0);
    expect_eq(parseExfatBootSector(bad_fats[0..]) == null, true);
    bad_fats[110] = 3;
    expect_eq(parseExfatBootSector(bad_fats[0..]) == null, true);

    // 7. Invalid root cluster (< 2)
    let bad_root: [512]u8 = std.mem.zeroes([512]u8);
    initExfatBoot(bad_root[0..], 9, 6, 2, 0);
    putU32Le(bad_root[0..], 96, 1);
    expect_eq(parseExfatBootSector(bad_root[0..]) == null, true);
});

test("6. Malformed directory entries & small output buffer guards", () => {
    let raw: [32]u8 = std.mem.zeroes([32]u8);

    // Wrong entry types
    raw[0] = ENTRY_STREAM_EXT;
    expect_eq(parseFileEntry(raw[0..]) == null, true);

    raw[0] = ENTRY_FILE;
    raw[1] = 1; // secondary_count < 2 rejected
    expect_eq(parseFileEntry(raw[0..]) == null, true);
    expect_eq(parseStreamExt(raw[0..]) == null, true);

    // Truncated entry (< 32B)
    expect_eq(parseFileEntry(raw[0..16]) == null, true);
    expect_eq(parseStreamExt(raw[0..16]) == null, true);

    // decodeFileNameAscii with wrong type or small out buffer
    let small: [4]u8 = [0; 4];
    expect_eq(decodeFileNameAscii(raw[0..], small[0..]), 0 as usize);

    writeUtf16LeName(raw[0..], "LONGFILENAME.BIN");
    const clipped = decodeFileNameAscii(raw[0..], small[0..]);
    expect_eq(clipped, 4 as usize);
    expect_eq(eqStr(small[0..4], "LONG"), true);
});
