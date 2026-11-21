// Source: cp/cap/test/test_ntfs_production_deep.cp (Production deep tests for cap:ntfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NTFS_VBR_SIZE,
    NTFS_OEM_ID,
    NTFS_BOOT_SIGNATURE,
    NTFS_FILE_MAGIC,
    NTFS_ATTR_END,
    MFT_FLAG_IN_USE,
    MFT_FLAG_DIRECTORY,
    ATTR_STANDARD_INFO,
    ATTR_FILE_NAME,
    ATTR_DATA,
    NtfsVbr,
    isNtfsVbr,
    parseNtfsVbr,
    NtfsDataRun,
    NtfsRunIter,
    NtfsAttr,
    NtfsAttrIter,
    NtfsMftRecord,
    parseNtfsMftRecord,
} from "cap:ntfs";

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

fn writeU64LeTest(buf: []u8, off: usize, val: u64) {
    writeU32LeTest(buf, off, (val & 0xFFFFFFFF) as u32);
    writeU32LeTest(buf, off + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
}

test("1. 512B NTFS VBR parsing with negative clusters_per_mft_record (-10 -> 1024B)", () => {
    expect_eq(NTFS_VBR_SIZE, 512);
    expect_eq(NTFS_BOOT_SIGNATURE, 0xAA55 as u16);

    let vbr: [512]u8 = [0; 512];
    const oem = NTFS_OEM_ID;
    let i: usize = 0;
    while (i < 8) {
        vbr[3 + i] = oem[i];
        i += 1;
    }
    writeU16LeTest(vbr[0..], 11, 512);           // bytes_per_sector = 512
    vbr[13] = 8;                                 // sectors_per_cluster = 8 (4KB clusters)
    writeU64LeTest(vbr[0..], 40, 2097152);       // total_sectors = 1GB
    writeU64LeTest(vbr[0..], 48, 786432);        // mft_lcn
    writeU64LeTest(vbr[0..], 56, 2);             // mftmirr_lcn
    vbr[64] = 0xF6;                              // -10 as i8 -> 2^10 = 1024 bytes
    vbr[68] = 1;                                 // clusters_per_index_block = 1
    writeU64LeTest(vbr[0..], 72, 0x1122334455667788);
    writeU16LeTest(vbr[0..], 510, NTFS_BOOT_SIGNATURE);

    expect_eq(isNtfsVbr(vbr[0..]), true);
    const parsed = parseNtfsVbr(vbr[0..]) ?? unreachable;
    expect_eq(parsed.bytes_per_sector, 512 as u16);
    expect_eq(parsed.sectors_per_cluster, 8 as u8);
    expect_eq(parsed.clusterSizeBytes(), 4096 as u32);
    expect_eq(parsed.clusters_per_mft_record, -10 as i8);
    expect_eq(parsed.mftRecordSizeBytes(), 1024 as u32);
    expect_eq(parsed.mftByteOffset(), (786432 as u64) * 4096);
    expect_eq(parsed.volume_serial, 0x1122334455667788 as u64);
});

test("2. MFT FILE record with resident $STANDARD_INFORMATION and resident $DATA attributes", () => {
    let rec: [256]u8 = [0; 256];
    // "FILE" magic
    rec[0] = 'F';
    rec[1] = 'I';
    rec[2] = 'L';
    rec[3] = 'E';
    writeU16LeTest(rec[0..], 4, 48);  // usa_offset
    writeU16LeTest(rec[0..], 6, 3);   // usa_count
    writeU64LeTest(rec[0..], 8, 999); // lsn
    writeU16LeTest(rec[0..], 16, 2);  // seq_num
    writeU16LeTest(rec[0..], 18, 1);  // hard_link_count
    writeU16LeTest(rec[0..], 20, 56); // first_attr_offset
    writeU16LeTest(rec[0..], 22, MFT_FLAG_IN_USE);
    writeU32LeTest(rec[0..], 24, 152); // used_size
    writeU32LeTest(rec[0..], 28, 1024);

    // Attr 0 at offset 56: resident $STANDARD_INFORMATION (0x10), total_len=40 (24B hdr + 16B val)
    writeU32LeTest(rec[0..], 56, ATTR_STANDARD_INFO);
    writeU32LeTest(rec[0..], 60, 40);
    rec[64] = 0; // resident
    writeU32LeTest(rec[0..], 72, 16); // val_len
    writeU16LeTest(rec[0..], 76, 24); // val_off

    // Attr 1 at offset 96: resident $DATA (0x80), total_len=40, val="hello ntfs!" (11 bytes)
    writeU32LeTest(rec[0..], 96, ATTR_DATA);
    writeU32LeTest(rec[0..], 100, 40);
    rec[104] = 0; // resident
    writeU32LeTest(rec[0..], 112, 11);
    writeU16LeTest(rec[0..], 116, 24);
    const msg = "hello ntfs!";
    let i: usize = 0;
    while (i < msg.len) {
        rec[96 + 24 + i] = msg[i];
        i += 1;
    }

    // End marker at offset 136
    writeU32LeTest(rec[0..], 136, NTFS_ATTR_END);

    const parsed = parseNtfsMftRecord(rec[0..]) ?? unreachable;
    expect_eq(parsed.isInUse(), true);
    expect_eq(parsed.isDirectory(), false);
    expect_eq(parsed.seq_num, 2 as u16);

    let it = parsed.attrs();
    const a0 = it.next() ?? unreachable;
    expect_eq(a0.attr_type, ATTR_STANDARD_INFO);
    expect_eq(a0.non_resident, false);
    expect_eq(a0.resident_value.len, 16);

    const a1 = it.next() ?? unreachable;
    expect_eq(a1.attr_type, ATTR_DATA);
    expect_eq(a1.non_resident, false);
    expect_eq(a1.real_size, 11 as u64);
    expectBytesEq(a1.resident_value, "hello ntfs!");

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("3. Non-resident $DATA attribute & Data Runlist (positive delta, sparse run, negative delta LCN)", () => {
    // Runlist bytes:
    // Run 0: hdr=0x21 (len_sz=1, off_sz=2): len=4, delta=+0x0100 (+256) => LCN=256
    // Run 1: hdr=0x01 (len_sz=1, off_sz=0): len=8, sparse => is_sparse=true, prev_lcn stays 256
    // Run 2: hdr=0x11 (len_sz=1, off_sz=1): len=2, delta=-16 (0xF0) => LCN=240
    // Terminator: 0x00
    const runlist: [9]u8 = [
        0x21, 0x04, 0x00, 0x01,
        0x01, 0x08,
        0x11, 0x02, 0xF0,
    ];

    let rec: [256]u8 = [0; 256];
    rec[0] = 'F';
    rec[1] = 'I';
    rec[2] = 'L';
    rec[3] = 'E';
    writeU16LeTest(rec[0..], 20, 56);
    writeU16LeTest(rec[0..], 22, MFT_FLAG_IN_USE | MFT_FLAG_DIRECTORY);
    writeU32LeTest(rec[0..], 24, 152);

    // Non-resident $DATA at offset 56: total_len=80 (64B hdr + 16B runlist area)
    writeU32LeTest(rec[0..], 56, ATTR_DATA);
    writeU32LeTest(rec[0..], 60, 80);
    rec[64] = 1; // non_resident = true
    writeU16LeTest(rec[0..], 56 + 32, 64);      // runlist_offset = 64
    writeU64LeTest(rec[0..], 56 + 40, 57344);   // alloc_size = 14 * 4096
    writeU64LeTest(rec[0..], 56 + 48, 55000);   // real_size
    let i: usize = 0;
    while (i < 9) {
        rec[56 + 64 + i] = runlist[i];
        i += 1;
    }
    rec[56 + 64 + 9] = 0x00; // runlist terminator
    writeU32LeTest(rec[0..], 136, NTFS_ATTR_END);

    const parsed = parseNtfsMftRecord(rec[0..]) ?? unreachable;
    expect_eq(parsed.isDirectory(), true);
    let ait = parsed.attrs();
    const da = ait.next() ?? unreachable;
    expect_eq(da.non_resident, true);
    expect_eq(da.alloc_size, 57344 as u64);
    expect_eq(da.real_size, 55000 as u64);

    let rit = da.runs();
    const r0 = rit.next() ?? unreachable;
    expect_eq(r0.is_sparse, false);
    expect_eq(r0.cluster_count, 4 as u64);
    expect_eq(r0.lcn, 256 as i64);

    const r1 = rit.next() ?? unreachable;
    expect_eq(r1.is_sparse, true);
    expect_eq(r1.cluster_count, 8 as u64);
    expect_eq(r1.lcn, 0 as i64);

    const r2 = rit.next() ?? unreachable;
    expect_eq(r2.is_sparse, false);
    expect_eq(r2.cluster_count, 2 as u64);
    expect_eq(r2.lcn, 240 as i64);

    expect_eq(rit.next() == null, true);
    expect_eq(rit.hasError(), false);
});

test("4. Direct NtfsRunIter 3-byte signed negative delta & truncation error detection", () => {
    // Run 0: len=1, delta=+100000 (0x0186A0 -> 3 bytes LE: A0 86 01)
    // Run 1: len=3, delta=-70000 (-70000 in 24-bit two's complement = 0xFEEA90 -> LE: 90 EA FE) => LCN=30000
    const runs_raw: [11]u8 = [
        0x31, 0x01, 0xA0, 0x86, 0x01,
        0x31, 0x03, 0x90, 0xEA, 0xFE,
        0x00,
    ];
    let it = NtfsRunIter.init(runs_raw[0..]);
    const r0 = it.next() ?? unreachable;
    expect_eq(r0.lcn, 100000 as i64);
    const r1 = it.next() ?? unreachable;
    expect_eq(r1.cluster_count, 3 as u64);
    expect_eq(r1.lcn, 30000 as i64);
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);

    // Truncated runlist sets hasError()
    const trunc: [2]u8 = [0x21, 0x04];
    let bad_it = NtfsRunIter.init(trunc[0..]);
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.hasError(), true);
});

test("5. Malformed VBR, MFT magic, attribute total_len<16 & truncation rejection", () => {
    // 1. Wrong OEM ID in VBR
    let bad_vbr: [512]u8 = [0; 512];
    bad_vbr[3] = 'F';
    bad_vbr[4] = 'A';
    bad_vbr[5] = 'T';
    writeU16LeTest(bad_vbr[0..], 11, 512);
    bad_vbr[13] = 8;
    writeU64LeTest(bad_vbr[0..], 40, 1000);
    bad_vbr[64] = 0xF6;
    writeU16LeTest(bad_vbr[0..], 510, NTFS_BOOT_SIGNATURE);
    expect_eq(isNtfsVbr(bad_vbr[0..]), false);
    expect_eq(parseNtfsVbr(bad_vbr[0..]) == null, true);

    // 2. Non-"FILE" magic in MFT record
    let bad_mft: [64]u8 = [0; 64];
    bad_mft[0] = 'B';
    bad_mft[1] = 'A';
    bad_mft[2] = 'A';
    bad_mft[3] = 'D';
    expect_eq(parseNtfsMftRecord(bad_mft[0..]) == null, true);

    // 3. Attribute total_len < 16 (zero-length infinite loop guard)
    let loop_mft: [96]u8 = [0; 96];
    loop_mft[0] = 'F';
    loop_mft[1] = 'I';
    loop_mft[2] = 'L';
    loop_mft[3] = 'E';
    writeU16LeTest(loop_mft[0..], 20, 56);
    writeU32LeTest(loop_mft[0..], 24, 96);
    writeU32LeTest(loop_mft[0..], 56, ATTR_DATA);
    writeU32LeTest(loop_mft[0..], 60, 8); // invalid < 16!

    const rec = parseNtfsMftRecord(loop_mft[0..]) ?? unreachable;
    let it = rec.attrs();
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), true);

    // 4. Truncated MFT record (< 48B)
    expect_eq(parseNtfsMftRecord(loop_mft[0..32]) == null, true);
});
