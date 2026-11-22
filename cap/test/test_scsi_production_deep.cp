// Source: cp/cap/test/test_scsi_production_deep.cp (Production deep tests for cap:scsi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OP_TEST_UNIT_READY,
    OP_REQUEST_SENSE,
    OP_INQUIRY,
    OP_READ_CAPACITY_10,
    OP_READ_10,
    OP_WRITE_10,
    OP_SYNCHRONIZE_CACHE_10,
    OP_READ_16,
    OP_WRITE_16,
    OP_SERVICE_ACTION_IN_16,
    STATUS_GOOD,
    STATUS_CHECK_CONDITION,
    STATUS_BUSY,
    STATUS_RESERVATION_CONFLICT,
    STATUS_TASK_SET_FULL,
    SK_NO_SENSE,
    SK_RECOVERED_ERROR,
    SK_NOT_READY,
    SK_MEDIUM_ERROR,
    SK_HARDWARE_ERROR,
    SK_ILLEGAL_REQUEST,
    SK_UNIT_ATTENTION,
    SK_DATA_PROTECT,
    SK_ABORTED_COMMAND,
    PDT_DIRECT_ACCESS,
    PDT_SEQUENTIAL_ACCESS,
    PDT_CD_DVD,
    PDT_ENCLOSURE,
    cdbLengthForOpcode,
    ScsiInquiryData,
    parseInquiry,
    ScsiCapacity,
    parseReadCapacity10,
    parseReadCapacity16,
    ScsiSense,
    parseSense,
    encodeInquiryCdb,
    encodeRead10Cdb,
    encodeWrite10Cdb,
    encodeRead16Cdb,
    encodeWrite16Cdb,
} from "cap:sys";

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

test("1. CDB opcode length table (6/10/12/16B) and INQUIRY/READ10/WRITE10/READ16 encoding", () => {
    // Group 0 -> 6, Group 1/2 -> 10, Group 4 -> 16, Group 5 -> 12, Group 3/6/7 -> null
    expect_eq(cdbLengthForOpcode(OP_TEST_UNIT_READY) ?? 0, 6 as u8);
    expect_eq(cdbLengthForOpcode(OP_INQUIRY) ?? 0, 6 as u8);
    expect_eq(cdbLengthForOpcode(OP_READ_10) ?? 0, 10 as u8);
    expect_eq(cdbLengthForOpcode(OP_SYNCHRONIZE_CACHE_10) ?? 0, 10 as u8);
    expect_eq(cdbLengthForOpcode(OP_READ_16) ?? 0, 16 as u8);
    expect_eq(cdbLengthForOpcode(OP_SERVICE_ACTION_IN_16) ?? 0, 16 as u8);
    expect_eq(cdbLengthForOpcode(0xA8) ?? 0, 12 as u8); // READ(12)
    expect_eq(cdbLengthForOpcode(0x7F) == null, true);
    expect_eq(cdbLengthForOpcode(0xC0) == null, true);

    let cdb: [16]u8 = std.mem.zeroes([16]u8);

    // INQUIRY (alloc_len = 96 = 0x0060)
    const n_inq = encodeInquiryCdb(96, cdb[0..]) ?? unreachable;
    expect_eq(n_inq, 6 as usize);
    expect_eq(cdb[0], OP_INQUIRY);
    expect_eq(cdb[3], 0x00 as u8);
    expect_eq(cdb[4], 0x60 as u8);

    // READ(10) LBA=0x01020304, blocks=0x0080
    const n_r10 = encodeRead10Cdb(0x01020304, 0x0080, cdb[0..]) ?? unreachable;
    expect_eq(n_r10, 10 as usize);
    expect_eq(cdb[0], OP_READ_10);
    expect_eq(cdb[2], 0x01 as u8);
    expect_eq(cdb[5], 0x04 as u8);
    expect_eq(cdb[7], 0x00 as u8);
    expect_eq(cdb[8], 0x80 as u8);

    // WRITE(10)
    const n_w10 = encodeWrite10Cdb(0xAABBCCDD, 16, cdb[0..]) ?? unreachable;
    expect_eq(n_w10, 10 as usize);
    expect_eq(cdb[0], OP_WRITE_10);
    expect_eq(cdb[2], 0xAA as u8);
    expect_eq(cdb[8], 16 as u8);

    // READ(16) LBA=0x00000002_00000001 (>2TB), blocks=256
    const n_r16 = encodeRead16Cdb(0x00000002_00000001, 256, cdb[0..]) ?? unreachable;
    expect_eq(n_r16, 16 as usize);
    expect_eq(cdb[0], OP_READ_16);
    expect_eq(cdb[5], 0x02 as u8);
    expect_eq(cdb[9], 0x01 as u8);
    expect_eq(cdb[12], 0x01 as u8);
    expect_eq(cdb[13], 0x00 as u8);
});

test("2. 36-byte Standard INQUIRY parsing and trailing space trimming", () => {
    let inq: [36]u8 = std.mem.zeroes([36]u8);
    inq[0] = PDT_DIRECT_ACCESS; // PQ=0, PDT=0
    inq[1] = 0x80;              // RMB=1 (removable)
    inq[2] = 0x06;              // SPC-4
    inq[3] = 0x02;              // standard response format 2
    putBytes(inq[0..], 8, "SAMSUNG ");
    putBytes(inq[0..], 16, "PM9A3 NVMe SSD  ");
    putBytes(inq[0..], 32, "1.0 ");

    const d = parseInquiry(inq[0..]) ?? unreachable;
    expect_eq(d.peripheral_qualifier, 0 as u8);
    expect_eq(d.device_type, PDT_DIRECT_ACCESS);
    expect_eq(d.removable, true);
    expect_eq(d.version, 0x06 as u8);
    expect_eq(d.response_format, 0x02 as u8);
    expect_eq(std.mem.eql(u8, d.vendorSlice(), "SAMSUNG"), true);
    expect_eq(std.mem.eql(u8, d.productSlice(), "PM9A3 NVMe SSD"), true);
    expect_eq(std.mem.eql(u8, d.revSlice(), "1.0"), true);
});

test("3. READ CAPACITY(10) and READ CAPACITY(16) >2TB 128-bit totalBytes calculation", () => {
    // READ CAPACITY(10): 1,000,000 sectors (last_lba = 999,999 = 0x000F423F), block_size = 512 (0x00000200)
    let cap10_raw: [8]u8 = [0x00, 0x0F, 0x42, 0x3F, 0x00, 0x00, 0x02, 0x00];
    const c10 = parseReadCapacity10(cap10_raw[0..]) ?? unreachable;
    expect_eq(c10.last_lba, 999999 as u64);
    expect_eq(c10.block_size, 512 as u32);
    expect_eq(c10.sectorCount(), 1000000 as u64);
    expect_eq(c10.totalBytes(), 512000000 as u128);

    // READ CAPACITY(16): 16 TB drive -> last_lba = (16 * 1024^4 / 4096) - 1 = 0x00000000_FFFFFFFF (4,294,967,295)
    // block_size = 4096 (0x00001000) -> totalBytes = 4,294,967,296 * 4096 = 17,592,186,044,416 (16 TiB)
    let cap16_raw: [12]u8 = [
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x10, 0x00,
    ];
    const c16 = parseReadCapacity16(cap16_raw[0..]) ?? unreachable;
    expect_eq(c16.sectorCount(), 4294967296 as u64);
    expect_eq(c16.block_size, 4096 as u32);
    expect_eq(c16.totalBytes(), 17592186044416 as u128);
});

test("4. Fixed (0x70/0x71) and Descriptor (0x72/0x73) Sense Data decoding", () => {
    // Fixed Current Sense (0x70 | 0x80 valid=1 = 0xF0):
    // sense_key = SK_MEDIUM_ERROR (0x03), info = 0x00001234, ASC = 0x11, ASCQ = 0x00 (Unrecovered read error)
    let fixed_cur: [18]u8 = std.mem.zeroes([18]u8);
    fixed_cur[0] = 0xF0;
    fixed_cur[2] = SK_MEDIUM_ERROR;
    fixed_cur[3] = 0x00;
    fixed_cur[4] = 0x00;
    fixed_cur[5] = 0x12;
    fixed_cur[6] = 0x34;
    fixed_cur[12] = 0x11;
    fixed_cur[13] = 0x00;

    const s1 = parseSense(fixed_cur[0..]) ?? unreachable;
    expect_eq(s1.response_code, 0x70 as u8);
    expect_eq(s1.is_descriptor, false);
    expect_eq(s1.isCurrent(), true);
    expect_eq(s1.isOk(), false);
    expect_eq(s1.valid_info, true);
    expect_eq(s1.info, 0x00001234 as u32);
    expect_eq(s1.sense_key, SK_MEDIUM_ERROR);
    expect_eq(s1.asc, 0x11 as u8);
    expect_eq(s1.ascq, 0x00 as u8);

    // Fixed Deferred Sense (0x71): SK_NO_SENSE
    let fixed_def: [14]u8 = std.mem.zeroes([14]u8);
    fixed_def[0] = 0x71;
    fixed_def[2] = SK_NO_SENSE;
    const s2 = parseSense(fixed_def[0..]) ?? unreachable;
    expect_eq(s2.is_deferred, true);
    expect_eq(s2.isCurrent(), false);
    expect_eq(s2.isOk(), true);

    // Descriptor Current Sense (0x72): SK_ILLEGAL_REQUEST (0x05), ASC=0x24, ASCQ=0x00 (Invalid field in CDB)
    let desc_cur: [8]u8 = [0x72, SK_ILLEGAL_REQUEST, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00];
    const s3 = parseSense(desc_cur[0..]) ?? unreachable;
    expect_eq(s3.response_code, 0x72 as u8);
    expect_eq(s3.is_descriptor, true);
    expect_eq(s3.isCurrent(), true);
    expect_eq(s3.sense_key, SK_ILLEGAL_REQUEST);
    expect_eq(s3.asc, 0x24 as u8);
    expect_eq(s3.ascq, 0x00 as u8);

    // Descriptor Deferred Sense (0x73): SK_UNIT_ATTENTION (0x06)
    let desc_def: [8]u8 = [0x73, SK_UNIT_ATTENTION, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00];
    const s4 = parseSense(desc_def[0..]) ?? unreachable;
    expect_eq(s4.is_descriptor, true);
    expect_eq(s4.is_deferred, true);
    expect_eq(s4.sense_key, SK_UNIT_ATTENTION);
});

test("5. Malformed response_code, truncated payloads, and tiny CDB output buffers rejected", () => {
    // Invalid Sense response_code (0x6F, 0x74) and truncated buffers
    let bad_rc: [14]u8 = std.mem.zeroes([14]u8);
    bad_rc[0] = 0x74;
    expect_eq(parseSense(bad_rc[0..]) == null, true);

    // Fixed sense requires >= 14 bytes (12 bytes must be rejected)
    let short_fixed: [12]u8 = std.mem.zeroes([12]u8);
    short_fixed[0] = 0x70;
    expect_eq(parseSense(short_fixed[0..]) == null, true);

    // Descriptor sense requires >= 8 bytes
    let short_desc: [7]u8 = [0x72, 0x05, 0x24, 0x00, 0x00, 0x00, 0x00];
    expect_eq(parseSense(short_desc[0..]) == null, true);

    // Truncated INQUIRY / READ CAPACITY(10/16)
    let short_inq: [35]u8 = std.mem.zeroes([35]u8);
    expect_eq(parseInquiry(short_inq[0..]) == null, true);
    expect_eq(parseReadCapacity10(short_inq[0..7]) == null, true);
    expect_eq(parseReadCapacity16(short_inq[0..11]) == null, true);

    // Tiny CDB output buffers rejected
    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(encodeInquiryCdb(36, tiny[0..]) == null, true);
    expect_eq(encodeRead10Cdb(0, 1, tiny[0..]) == null, true);
    expect_eq(encodeWrite10Cdb(0, 1, tiny[0..]) == null, true);
    expect_eq(encodeRead16Cdb(0, 1, tiny[0..]) == null, true);
    expect_eq(encodeWrite16Cdb(0, 1, tiny[0..]) == null, true);
});
