// Source: cp/cap/test/test_vhdx_production_deep.cp (Production deep tests for cap:vhdx)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VHDX_FILE_ID_SIG,
    VHDX_HEADER_SIG,
    VHDX_REGION_SIG,
    VHDX_METADATA_SIG,
    VHDX_HEADER_SIZE,
    VHDX_REGION_SIZE,
    VHDX_HEADER1_OFFSET,
    VHDX_HEADER2_OFFSET,
    VHDX_REGION1_OFFSET,
    REGION_GUID_BAT,
    REGION_GUID_METADATA,
    BAT_NOT_PRESENT,
    BAT_UNDEFINED,
    BAT_ZERO,
    BAT_UNMAPPED,
    BAT_FULLY_PRESENT,
    BAT_PARTIALLY_PRESENT,
    vhdxCrc32c,
    vhdxCrc32cWithZeroField,
    isVhdx,
    parseVhdxFileId,
    parseVhdxHeader,
    selectActiveHeader,
    parseVhdxRegionTable,
    decodeVhdxBatEntry,
} from "cap:sys";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn buildHeaderBlock(buf: []u8, seq: u64, version: u16, log_len: u32, log_off: u64, seed: u8) {
    let i: usize = 0;
    while (i < VHDX_HEADER_SIZE) {
        buf[i] = 0;
        i += 1;
    }
    copyInto(buf, 0, VHDX_HEADER_SIG);
    writeU64Le(buf, 8, seq);
    // FileWriteGuid / DataWriteGuid / LogGuid
    let g: usize = 0;
    while (g < 16) {
        buf[16 + g] = seed + (g as u8);
        buf[32 + g] = seed + 0x20 + (g as u8);
        buf[48 + g] = seed + 0x40 + (g as u8);
        g += 1;
    }
    writeU16Le(buf, 64, 0); // log_version
    writeU16Le(buf, 66, version);
    writeU32Le(buf, 68, log_len);
    writeU64Le(buf, 72, log_off);

    const crc = vhdxCrc32cWithZeroField(buf[0..VHDX_HEADER_SIZE], 4);
    writeU32Le(buf, 4, crc);
}

test("1. VHDX File Type Identifier signature, UTF-16LE creator decode, and CRC32C standard vector", () => {
    // Verify protocol constants
    expect_eq(VHDX_HEADER_SIZE, 4096);
    expect_eq(VHDX_REGION_SIZE, 65536);
    expect_eq(VHDX_HEADER1_OFFSET, 65536);
    expect_eq(VHDX_HEADER2_OFFSET, 131072);
    expect_eq(VHDX_REGION1_OFFSET, 196608);
    expect_eq(std.mem.eql(u8, VHDX_METADATA_SIG, "metadata"), true);

    // RFC 3720 / iSCSI standard CRC32C check value for "123456789" == 0xE3069283
    expect_eq(vhdxCrc32c("123456789"), 0xE3069283 as u32);
    expect_eq(vhdxCrc32c(""), 0 as u32);

    // Build 520-byte File Type Identifier with UTF-16LE "CP-VHDX" + non-ASCII U+4E2D + NUL
    let fid_buf: [520]u8 = [0; 520];
    copyInto(fid_buf[0..], 0, VHDX_FILE_ID_SIG);
    const ascii_name = "CP-VHDX";
    let i: usize = 0;
    while (i < ascii_name.len) {
        writeU16Le(fid_buf[0..], 8 + i * 2, ascii_name[i] as u16);
        i += 1;
    }
    // Append a non-ASCII UTF-16LE code unit (0x4E2D) followed by NUL terminator
    writeU16Le(fid_buf[0..], 8 + ascii_name.len * 2, 0x4E2D);
    writeU16Le(fid_buf[0..], 8 + (ascii_name.len + 1) * 2, 0x0000);

    expect_eq(isVhdx(fid_buf[0..]), true);
    const fid = parseVhdxFileId(fid_buf[0..]) ?? unreachable;
    expect_eq(fid.creator_utf16le.len, 512);

    let out_full: [32]u8 = [0; 32];
    const n_full = fid.decodeCreatorAscii(out_full[0..]);
    expect_eq(n_full, 8);
    expect_eq(std.mem.eql(u8, out_full[0..n_full], "CP-VHDX?"), true);

    // Output buffer truncation boundary
    let out_small: [4]u8 = [0; 4];
    const n_small = fid.decodeCreatorAscii(out_small[0..]);
    expect_eq(n_small, 4);
    expect_eq(std.mem.eql(u8, out_small[0..n_small], "CP-V"), true);
});

test("2. Dual 4KB VHDX Headers CRC32C verification and selectActiveHeader election", () => {
    let h1_buf: [4096]u8 = [0; 4096];
    let h2_buf: [4096]u8 = [0; 4096];

    buildHeaderBlock(h1_buf[0..], 101, 1, 1048576, 1048576, 0x10);
    buildHeaderBlock(h2_buf[0..], 205, 1, 2097152, 2097152, 0x20);

    const h1 = parseVhdxHeader(h1_buf[0..]);
    const h2 = parseVhdxHeader(h2_buf[0..]);
    expect_eq(h1 != null, true);
    expect_eq(h2 != null, true);

    // Verify fields of h1
    const h1_val = h1 ?? unreachable;
    expect_eq(h1_val.sequence_number, 101 as u64);
    expect_eq(h1_val.version, 1 as u16);
    expect_eq(h1_val.log_version, 0 as u16);
    expect_eq(h1_val.log_length, 1048576 as u32);
    expect_eq(h1_val.log_offset, 1048576 as u64);
    expect_eq(h1_val.file_write_guid[0], 0x10 as u8);
    expect_eq(h1_val.data_write_guid[0], 0x30 as u8);
    expect_eq(h1_val.log_guid[0], 0x50 as u8);

    // Both valid: higher sequence_number wins (h2: 205 > h1: 101)
    const active_both = selectActiveHeader(h1, h2) ?? unreachable;
    expect_eq(active_both.sequence_number, 205 as u64);
    expect_eq(active_both.log_length, 2097152 as u32);

    // Reverse order: h1 higher than h2
    const active_rev = selectActiveHeader(h2, h1) ?? unreachable;
    expect_eq(active_rev.sequence_number, 205 as u64);

    // Only one valid header
    const active_only1 = selectActiveHeader(h1, null) ?? unreachable;
    expect_eq(active_only1.sequence_number, 101 as u64);
    const active_only2 = selectActiveHeader(null, h2) ?? unreachable;
    expect_eq(active_only2.sequence_number, 205 as u64);

    // Neither valid
    expect_eq(selectActiveHeader(null, null) == null, true);
});

test("3. 64KB Region Table CRC32C verification, iterator traversal, and BAT/Metadata GUID lookup", () => {
    let reg_buf: [65536]u8 = [0; 65536];
    copyInto(reg_buf[0..], 0, VHDX_REGION_SIG);
    writeU32Le(reg_buf[0..], 8, 3); // entry_count = 3 (BAT, custom, Metadata)
    writeU32Le(reg_buf[0..], 12, 0);

    // Entry 0: BAT region at offset 3MB, length 1MB, required=true
    copyInto(reg_buf[0..], 16, REGION_GUID_BAT[0..]);
    writeU64Le(reg_buf[0..], 16 + 16, 3145728);
    writeU32Le(reg_buf[0..], 16 + 24, 1048576);
    writeU32Le(reg_buf[0..], 16 + 28, 1);

    // Entry 1: Custom non-required region
    let custom_guid: [16]u8 = [0xAA; 16];
    copyInto(reg_buf[0..], 48, custom_guid[0..]);
    writeU64Le(reg_buf[0..], 48 + 16, 4194304);
    writeU32Le(reg_buf[0..], 48 + 24, 65536);
    writeU32Le(reg_buf[0..], 48 + 28, 0);

    // Entry 2: Metadata region at offset 2MB, length 1MB, required=true
    copyInto(reg_buf[0..], 80, REGION_GUID_METADATA[0..]);
    writeU64Le(reg_buf[0..], 80 + 16, 2097152);
    writeU32Le(reg_buf[0..], 80 + 24, 1048576);
    writeU32Le(reg_buf[0..], 80 + 28, 1);

    const crc = vhdxCrc32cWithZeroField(reg_buf[0..VHDX_REGION_SIZE], 4);
    writeU32Le(reg_buf[0..], 4, crc);

    const table = parseVhdxRegionTable(reg_buf[0..]) ?? unreachable;
    expect_eq(table.entry_count, 3 as u32);
    expect_eq(table.checksum, crc);

    // Iterate all entries
    let iter = table.entries();
    let count: usize = 0;
    let required_count: usize = 0;
    while (entry := iter.next()) {
        count += 1;
        if (entry.required) {
            required_count += 1;
        }
    }
    expect_eq(count, 3);
    expect_eq(required_count, 2);

    // Lookup well-known BAT & Metadata regions
    const bat = table.findBat() ?? unreachable;
    expect_eq(bat.isBat(), true);
    expect_eq(bat.isMetadata(), false);
    expect_eq(bat.file_offset, 3145728 as u64);
    expect_eq(bat.length, 1048576 as u32);
    expect_eq(bat.required, true);

    const meta = table.findMetadata() ?? unreachable;
    expect_eq(meta.isMetadata(), true);
    expect_eq(meta.isBat(), false);
    expect_eq(meta.file_offset, 2097152 as u64);
    expect_eq(meta.length, 1048576 as u32);
    expect_eq(meta.required, true);
});

test("4. 64-bit BAT entry state and 44-bit MB offset decoding", () => {
    // Fully present block at 42 MB
    const raw_present: u64 = ((42 as u64) << 20) | (BAT_FULLY_PRESENT as u64);
    const e_present = decodeVhdxBatEntry(raw_present);
    expect_eq(e_present.state, BAT_FULLY_PRESENT);
    expect_eq(e_present.file_offset_mb, 42 as u64);
    expect_eq(e_present.fileOffsetBytes(), (42 as u64) * 1048576);
    expect_eq(e_present.isPresent(), true);
    expect_eq(e_present.isZero(), false);

    // Partially present block at 1024 MB (1 GiB) with reserved bits set in bits 3..19
    const raw_partial: u64 = ((1024 as u64) << 20) | (0x1234 << 3) | (BAT_PARTIALLY_PRESENT as u64);
    const e_partial = decodeVhdxBatEntry(raw_partial);
    expect_eq(e_partial.state, BAT_PARTIALLY_PRESENT);
    expect_eq(e_partial.file_offset_mb, 1024 as u64);
    expect_eq(e_partial.fileOffsetBytes(), 1073741824 as u64);
    expect_eq(e_partial.isPresent(), true);
    expect_eq(e_partial.isZero(), false);

    // Zero block
    const e_zero = decodeVhdxBatEntry(BAT_ZERO as u64);
    expect_eq(e_zero.state, BAT_ZERO);
    expect_eq(e_zero.file_offset_mb, 0 as u64);
    expect_eq(e_zero.fileOffsetBytes(), 0 as u64);
    expect_eq(e_zero.isPresent(), false);
    expect_eq(e_zero.isZero(), true);

    // Not present / Undefined / Unmapped
    const e_not_present = decodeVhdxBatEntry(BAT_NOT_PRESENT as u64);
    expect_eq(e_not_present.isPresent(), false);
    expect_eq(e_not_present.isZero(), false);

    const e_undef = decodeVhdxBatEntry(BAT_UNDEFINED as u64);
    expect_eq(e_undef.state, BAT_UNDEFINED);
    expect_eq(e_undef.isPresent(), false);

    const e_unmapped = decodeVhdxBatEntry(BAT_UNMAPPED as u64);
    expect_eq(e_unmapped.state, BAT_UNMAPPED);
    expect_eq(e_unmapped.isPresent(), false);
});

test("5. Malformed input rejection: bad signatures, bad CRC32C, version != 1, entry_count > 2047, truncated buffers", () => {
    // Bad File ID signature & short buffer
    expect_eq(isVhdx("notavhdx"), false);
    expect_eq(isVhdx("vhdx"), false);
    expect_eq(parseVhdxFileId("notavhdxfile") == null, true);

    // Header: truncated buffer
    let short_hdr: [128]u8 = [0; 128];
    copyInto(short_hdr[0..], 0, VHDX_HEADER_SIG);
    expect_eq(parseVhdxHeader(short_hdr[0..]) == null, true);

    // Header: bad signature
    let bad_hdr: [4096]u8 = [0; 4096];
    buildHeaderBlock(bad_hdr[0..], 1, 1, 1048576, 1048576, 1);
    bad_hdr[0] = 0x58; // 'X' instead of 'h'
    expect_eq(parseVhdxHeader(bad_hdr[0..]) == null, true);

    // Header: corrupted CRC32C (single bit flip in payload)
    buildHeaderBlock(bad_hdr[0..], 1, 1, 1048576, 1048576, 1);
    bad_hdr[100] ^= 0x01;
    expect_eq(parseVhdxHeader(bad_hdr[0..]) == null, true);

    // Header: invalid version != 1 (even with valid CRC32C)
    buildHeaderBlock(bad_hdr[0..], 1, 2, 1048576, 1048576, 1);
    expect_eq(parseVhdxHeader(bad_hdr[0..]) == null, true);
    buildHeaderBlock(bad_hdr[0..], 1, 0, 1048576, 1048576, 1);
    expect_eq(parseVhdxHeader(bad_hdr[0..]) == null, true);

    // Region Table: truncated buffer
    let short_reg: [64]u8 = [0; 64];
    copyInto(short_reg[0..], 0, VHDX_REGION_SIG);
    expect_eq(parseVhdxRegionTable(short_reg[0..]) == null, true);

    // Region Table: bad signature
    let bad_reg: [65536]u8 = [0; 65536];
    copyInto(bad_reg[0..], 0, "bad!");
    writeU32Le(bad_reg[0..], 4, vhdxCrc32cWithZeroField(bad_reg[0..], 4));
    expect_eq(parseVhdxRegionTable(bad_reg[0..]) == null, true);

    // Region Table: entry_count > 2047 (2048 exceeds 64KB table capacity, even with valid CRC32C)
    copyInto(bad_reg[0..], 0, VHDX_REGION_SIG);
    writeU32Le(bad_reg[0..], 8, 2048);
    writeU32Le(bad_reg[0..], 4, vhdxCrc32cWithZeroField(bad_reg[0..], 4));
    expect_eq(parseVhdxRegionTable(bad_reg[0..]) == null, true);

    // Region Table: valid entry_count=0 but corrupted CRC32C
    writeU32Le(bad_reg[0..], 8, 0);
    const valid_crc = vhdxCrc32cWithZeroField(bad_reg[0..], 4);
    writeU32Le(bad_reg[0..], 4, valid_crc ^ 0xFF);
    expect_eq(parseVhdxRegionTable(bad_reg[0..]) == null, true);
});
