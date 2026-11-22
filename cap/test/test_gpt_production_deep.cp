// Source: cp/cap/test/test_gpt_production_deep.cp (Production deep tests for cap:gpt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GPT_SIGNATURE,
    GPT_REVISION_1_0,
    GPT_MIN_HEADER_SIZE,
    GPT_MIN_ENTRY_SIZE,
    MBR_SECTOR_SIZE,
    MBR_BOOT_SIGNATURE,
    MBR_TYPE_GPT_PROTECTIVE,
    GUID_UNUSED,
    GUID_EFI_SYSTEM,
    GUID_LINUX_FS,
    GUID_LINUX_SWAP,
    GUID_MS_BASIC_DATA,
    gptCrc32,
    formatGuid,
    MbrPartition,
    MbrPartitionTable,
    parseMbrPartitions,
    isProtectiveMbr,
    GptHeader,
    isGptHeader,
    parseGptHeader,
    verifyGptEntriesCrc,
    GptEntry,
    GptEntryIter,
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

fn putUtf16LeAscii(buf: []u8, off: usize, text: []const u8) {
    let i: usize = 0;
    while (i < text.len) {
        buf[off + i * 2] = text[i];
        buf[off + i * 2 + 1] = 0;
        i += 1;
    }
}

test("1. Protective MBR recognition and MBR partition table parsing", () => {
    expect_eq(MBR_SECTOR_SIZE, 512 as usize);
    expect_eq(MBR_BOOT_SIGNATURE, 0xAA55 as u16);
    expect_eq(MBR_TYPE_GPT_PROTECTIVE, 0xEE as u8);

    let sector0: [512]u8 = std.mem.zeroes([512]u8);
    // Partition 0: Protective GPT (type=0xEE, start_lba=1, sectors=0xFFFFFFFF)
    sector0[446 + 4] = MBR_TYPE_GPT_PROTECTIVE;
    putU32Le(sector0[0..], 446 + 8, 1);
    putU32Le(sector0[0..], 446 + 12, 0xFFFFFFFF);
    putU16Le(sector0[0..], 510, MBR_BOOT_SIGNATURE);

    expect_eq(isProtectiveMbr(sector0[0..]), true);
    const parts = parseMbrPartitions(sector0[0..]) ?? unreachable;
    expect_eq(parts[0].part_type, MBR_TYPE_GPT_PROTECTIVE);
    expect_eq(parts[0].start_lba, 1 as u32);
    expect_eq(parts[0].sector_count, 0xFFFFFFFF as u32);
    expect_eq(parts[1].part_type, 0 as u8);

    // Hybrid MBR (extra non-zero legacy partition) is not a pure Protective MBR
    sector0[446 + 16 + 4] = 0x83;
    expect_eq(isProtectiveMbr(sector0[0..]), false);

    // Missing 0xAA55 signature rejected
    sector0[510] = 0x00;
    expect_eq(parseMbrPartitions(sector0[0..]) == null, true);
});

test("2. IEEE 802.3 CRC-32 standard check vector and UEFI mixed-endian GUID 36-byte formatting", () => {
    // Standard IEEE CRC-32 check value: "123456789" -> 0xCBF43926
    expect_eq(gptCrc32("123456789"), 0xCBF43926 as u32);

    let out: [36]u8 = std.mem.zeroes([36]u8);

    const n1 = formatGuid(GUID_EFI_SYSTEM, out[0..]) ?? unreachable;
    expect_eq(n1, 36 as usize);
    expect_eq(std.mem.eql(u8, out[0..n1], "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"), true);

    const n2 = formatGuid(GUID_LINUX_FS, out[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, out[0..n2], "0FC63DAF-8483-4772-8E79-3D69D8477DE4"), true);

    const n3 = formatGuid(GUID_LINUX_SWAP, out[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, out[0..n3], "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F"), true);

    const n4 = formatGuid(GUID_MS_BASIC_DATA, out[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, out[0..n4], "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"), true);

    const n5 = formatGuid(GUID_UNUSED, out[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, out[0..n5], "00000000-0000-0000-0000-000000000000"), true);

    // Output buffer < 36 bytes rejected
    let small: [35]u8 = std.mem.zeroes([35]u8);
    expect_eq(formatGuid(GUID_EFI_SYSTEM, small[0..]) == null, true);
});

test("3. Full 92B GPT Header + 4-entry array dual CRC32 verification and GptEntryIter decoding", () => {
    // Build 4 entries (4 * 128 = 512 bytes):
    // Entry 0: EFI System Partition (LBA 2048..1050623, attr=Required|NoBlockIO=0x03, name="EFI System")
    // Entry 1: Unused (all zeros)
    // Entry 2: Linux FS (LBA 1050624..20971519, attr=0, name="Linux Root")
    // Entry 3: Unused
    let entries: [512]u8 = std.mem.zeroes([512]u8);

    // Entry 0 at offset 0
    putBytes(entries[0..], 0, GUID_EFI_SYSTEM[0..]);
    entries[16] = 0x11; // non-zero unique GUID
    putU64Le(entries[0..], 32, 2048);
    putU64Le(entries[0..], 40, 1050623);
    putU64Le(entries[0..], 48, 0x03);
    putUtf16LeAscii(entries[0..], 56, "EFI System");

    // Entry 2 at offset 256
    putBytes(entries[0..], 256, GUID_LINUX_FS[0..]);
    entries[256 + 16] = 0x22;
    putU64Le(entries[0..], 256 + 32, 1050624);
    putU64Le(entries[0..], 256 + 40, 20971519);
    putU64Le(entries[0..], 256 + 48, 0x00);
    putUtf16LeAscii(entries[0..], 256 + 56, "Linux Root");

    const entries_crc = gptCrc32(entries[0..]);

    // Build 92-byte GPT Header inside a 512-byte sector
    let lba1: [512]u8 = std.mem.zeroes([512]u8);
    putBytes(lba1[0..], 0, GPT_SIGNATURE);
    putU32Le(lba1[0..], 8, GPT_REVISION_1_0);
    putU32Le(lba1[0..], 12, 92);
    // bytes 16..20 left 0 while computing header CRC
    putU64Le(lba1[0..], 24, 1);         // current_lba
    putU64Le(lba1[0..], 32, 20971553);  // backup_lba
    putU64Le(lba1[0..], 40, 34);        // first_usable_lba
    putU64Le(lba1[0..], 48, 20971519);  // last_usable_lba
    putBytes(lba1[0..], 56, GUID_MS_BASIC_DATA[0..]); // disk_guid
    putU64Le(lba1[0..], 72, 2);         // part_entry_lba
    putU32Le(lba1[0..], 80, 4);         // num_part_entries
    putU32Le(lba1[0..], 84, 128);       // part_entry_size
    putU32Le(lba1[0..], 88, entries_crc);

    const hdr_crc = gptCrc32(lba1[0..92]);
    putU32Le(lba1[0..], 16, hdr_crc);

    expect_eq(isGptHeader(lba1[0..]), true);
    const hdr = parseGptHeader(lba1[0..]) ?? unreachable;
    expect_eq(hdr.revision, GPT_REVISION_1_0);
    expect_eq(hdr.header_size, 92 as u32);
    expect_eq(hdr.current_lba, 1 as u64);
    expect_eq(hdr.num_part_entries, 4 as u32);
    expect_eq(hdr.part_entry_size, 128 as u32);
    expect_eq(verifyGptEntriesCrc(hdr, entries[0..]), true);

    // Iterate skipping unused entries (should yield 2 partitions: EFI System & Linux Root)
    let it = GptEntryIter.initFromHeader(hdr, entries[0..], true);
    let name_buf: [64]u8 = std.mem.zeroes([64]u8);

    const p0 = it.next() ?? unreachable;
    expect_eq(p0.isUnused(), false);
    expect_eq(p0.isRequired(), true);
    expect_eq(p0.isNoBlockIo(), true);
    expect_eq(p0.sectorCount(), (1050623 - 2048 + 1) as u64);
    const n0 = p0.decodeNameAscii(name_buf[0..]);
    expect_eq(std.mem.eql(u8, name_buf[0..n0], "EFI System"), true);

    const p1 = it.next() ?? unreachable;
    expect_eq(p1.isUnused(), false);
    expect_eq(p1.isRequired(), false);
    expect_eq(p1.isNoBlockIo(), false);
    expect_eq(p1.sectorCount(), (20971519 - 1050624 + 1) as u64);
    const n1 = p1.decodeNameAscii(name_buf[0..]);
    expect_eq(std.mem.eql(u8, name_buf[0..n1], "Linux Root"), true);

    expect_eq(it.next() == null, true);

    // Iterate without skipping unused entries (should yield all 4)
    let it_all = GptEntryIter.initFromHeader(hdr, entries[0..], false);
    let total_seen: usize = 0;
    let unused_seen: usize = 0;
    while (true) {
        if (e := it_all.next()) {
            total_seen += 1;
            if (e.isUnused()) {
                unused_seen += 1;
                expect_eq(e.sectorCount(), 0 as u64);
            }
        } else {
            break;
        }
    }
    expect_eq(total_seen, 4 as usize);
    expect_eq(unused_seen, 2 as usize);
});

test("4. Extended entry size (256B) iteration and non-ASCII UTF-16LE fallback", () => {
    let entries: [512]u8 = std.mem.zeroes([512]u8);
    // Entry 0 at 0 (size 256)
    putBytes(entries[0..], 0, GUID_LINUX_SWAP[0..]);
    putU64Le(entries[0..], 32, 100);
    putU64Le(entries[0..], 40, 199);
    // "A" + non-ASCII U+4E2D + "B"
    entries[56] = 'A';
    entries[57] = 0x00;
    entries[58] = 0x2D;
    entries[59] = 0x4E;
    entries[60] = 'B';
    entries[61] = 0x00;

    // Entry 1 at 256 (size 256)
    putBytes(entries[0..], 256, GUID_MS_BASIC_DATA[0..]);
    putU64Le(entries[0..], 256 + 32, 200);
    putU64Le(entries[0..], 256 + 40, 299);

    let it = GptEntryIter.init(entries[0..], 2, 256, true);
    let name_buf: [16]u8 = std.mem.zeroes([16]u8);
    const e0 = it.next() ?? unreachable;
    expect_eq(e0.sectorCount(), 100 as u64);
    const nlen = e0.decodeNameAscii(name_buf[0..]);
    expect_eq(std.mem.eql(u8, name_buf[0..nlen], "A?B"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.sectorCount(), 100 as u64);
    expect_eq(it.next() == null, true);
});

test("5. Malformed GPT headers: bad signature, bad header CRC, bad entries CRC, header_size < 92 rejected", () => {
    // Build a valid 92B header first
    let hdr_buf: [92]u8 = std.mem.zeroes([92]u8);
    let entries: [128]u8 = std.mem.zeroes([128]u8);
    const ecrc = gptCrc32(entries[0..]);

    putBytes(hdr_buf[0..], 0, GPT_SIGNATURE);
    putU32Le(hdr_buf[0..], 8, GPT_REVISION_1_0);
    putU32Le(hdr_buf[0..], 12, 92);
    putU32Le(hdr_buf[0..], 80, 1);
    putU32Le(hdr_buf[0..], 84, 128);
    putU32Le(hdr_buf[0..], 88, ecrc);
    const hcrc = gptCrc32(hdr_buf[0..92]);
    putU32Le(hdr_buf[0..], 16, hcrc);

    const valid = parseGptHeader(hdr_buf[0..]) ?? unreachable;
    expect_eq(verifyGptEntriesCrc(valid, entries[0..]), true);

    // 1) Corrupted entries array fails verifyGptEntriesCrc
    entries[0] = 0xFF;
    expect_eq(verifyGptEntriesCrc(valid, entries[0..]), false);

    // 2) Corrupted header byte fails header CRC32
    let bad_crc = hdr_buf;
    bad_crc[24] ^= 0x01;
    expect_eq(parseGptHeader(bad_crc[0..]) == null, true);

    // 3) Bad signature rejected
    let bad_sig = hdr_buf;
    bad_sig[0] = 'X';
    expect_eq(isGptHeader(bad_sig[0..]), false);
    expect_eq(parseGptHeader(bad_sig[0..]) == null, true);

    // 4) header_size < 92 rejected (even with recomputed CRC)
    let small_hdr = hdr_buf;
    putU32Le(small_hdr[0..], 12, 88);
    putU32Le(small_hdr[0..], 16, 0);
    putU32Le(small_hdr[0..], 16, gptCrc32(small_hdr[0..88]));
    expect_eq(parseGptHeader(small_hdr[0..]) == null, true);

    // 5) part_entry_size < 128 rejected
    let small_ent = hdr_buf;
    putU32Le(small_ent[0..], 84, 64);
    putU32Le(small_ent[0..], 16, 0);
    putU32Le(small_ent[0..], 16, gptCrc32(small_ent[0..92]));
    expect_eq(parseGptHeader(small_ent[0..]) == null, true);

    // 6) Buffer shorter than 92 bytes rejected
    expect_eq(parseGptHeader(hdr_buf[0..64]) == null, true);
});
