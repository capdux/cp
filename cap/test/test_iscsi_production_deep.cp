// Source: cp/cap/test/test_iscsi_production_deep.cp (Production deep tests for cap:iscsi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ISCSI_TCP_PORT,
    ISCSI_BHS_SIZE,
    ISCSI_DIGEST_SIZE,
    OP_NOP_OUT,
    OP_SCSI_CMD,
    OP_LOGIN_REQ,
    OP_LOGIN_RESP,
    OP_DATA_IN,
    OP_REJECT,
    iscsiCrc32c,
    IscsiBhs,
    IscsiPdu,
    IscsiScsiCmd,
    IscsiLoginReq,
    IscsiKv,
    IscsiKvIter,
    parseIscsiPdu,
    parseIscsiScsiCmd,
    parseIscsiLoginReq,
    findIscsiKv,
} from "cap:iscsi";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn writeU32BeTest(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn writeU32LeTest(buf: []u8, off: usize, val: u32) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = ((val >> 16) & 0xFF) as u8;
    buf[off + 3] = ((val >> 24) & 0xFF) as u8;
}

test("1. Castagnoli CRC32C standard test vector and protocol constants", () => {
    expect_eq(ISCSI_TCP_PORT, 3260 as u16);
    expect_eq(ISCSI_BHS_SIZE, 48);
    expect_eq(ISCSI_DIGEST_SIZE, 4);

    // Official RFC 7143 / Castagnoli CRC32C check value for "123456789" is 0xE3069283
    expect_eq(iscsiCrc32c("123456789"), 0xE3069283 as u32);
    expect_eq(iscsiCrc32c(""), 0x00000000 as u32);
});

test("2. Login Request BHS + NUL-delimited Key=Value parameter iteration", () => {
    const kv_str = "InitiatorName=iqn.2026-01.org.cp:client\x00TargetName=iqn.2026-01.org.cp:disk0\x00AuthMethod=None\x00";
    // kv_str.len = 40 + 37 + 16 = 93 bytes -> padded to 96 bytes
    let buf: [160]u8 = [0; 160];
    buf[0] = (0x40 as u8) | OP_LOGIN_REQ;  // Immediate=1, Opcode=0x03
    buf[1] = 0x81;                         // T=1, C=0, CSG=0, NSG=1
    buf[2] = 0x00;                 // version_max
    buf[3] = 0x00;                 // version_min
    buf[4] = 0;                    // AHS words = 0
    // DataSegmentLength = 93 (0x00005D)
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = kv_str.len as u8;
    // ISID (6B) + TSIH (2B) at bytes 8..16
    buf[8] = 0x00;
    buf[9] = 0x02;
    buf[10] = 0x3D;
    buf[11] = 0x00;
    buf[12] = 0x00;
    buf[13] = 0x01;
    buf[14] = 0x00;
    buf[15] = 0x05; // TSIH = 5
    writeU32BeTest(buf[0..], 16, 0x00000042); // ITT
    buf[20] = 0x00;
    buf[21] = 0x01; // CID = 1
    writeU32BeTest(buf[0..], 24, 10); // CmdSN
    writeU32BeTest(buf[0..], 28, 1);  // ExpStatSN

    let i: usize = 0;
    while (i < kv_str.len) {
        buf[48 + i] = kv_str[i];
        i += 1;
    }

    const pdu = parseIscsiPdu(buf[0..144], false, false) ?? unreachable;
    expect_eq(pdu.bhs.immediate, true);
    expect_eq(pdu.bhs.opcode, OP_LOGIN_REQ);
    expect_eq(pdu.bhs.initiator_task_tag, 0x42 as u32);
    expect_eq(pdu.bhs.data_segment_len, kv_str.len as u32);
    expect_eq(pdu.bhs.paddedDataLen(), 96);
    expect_eq(pdu.consumed, 48 + 96);

    const login = parseIscsiLoginReq(pdu) ?? unreachable;
    expect_eq(login.transit, true);
    expect_eq(login.cont, false);
    expect_eq(login.csg, 0 as u8);
    expect_eq(login.nsg, 1 as u8);
    expect_eq(login.tsih, 5 as u16);
    expect_eq(login.cid, 1 as u16);
    expect_eq(login.cmd_sn, 10 as u32);
    expect_eq(login.exp_stat_sn, 1 as u32);
    expect_eq(parseIscsiScsiCmd(pdu) == null, true);

    expectBytesEq(findIscsiKv(pdu.data_segment, "InitiatorName") ?? unreachable, "iqn.2026-01.org.cp:client");
    expectBytesEq(findIscsiKv(pdu.data_segment, "TargetName") ?? unreachable, "iqn.2026-01.org.cp:disk0");
    expectBytesEq(findIscsiKv(pdu.data_segment, "AuthMethod") ?? unreachable, "None");
    expect_eq(findIscsiKv(pdu.data_segment, "MissingKey") == null, true);
});

test("3. SCSI Command BHS + 16B CDB + 4B-aligned Data Segment parsing", () => {
    let buf: [64]u8 = [0; 64];
    buf[0] = OP_SCSI_CMD;
    buf[1] = 0xC1;               // F=1, R=1, W=0, Attr=1 (Simple)
    buf[4] = 0;                  // AHS = 0
    buf[7] = 5;                  // DataSegmentLength = 5 ("abcde") -> padded to 8B
    buf[8] = 0x00;
    buf[9] = 0x01;               // LUN
    writeU32BeTest(buf[0..], 16, 0x10203040); // ITT
    writeU32BeTest(buf[0..], 20, 4096);       // ExpectedDataTransferLength
    writeU32BeTest(buf[0..], 24, 100);        // CmdSN
    writeU32BeTest(buf[0..], 28, 50);         // ExpStatSN
    // SCSI READ(10) CDB opcode 0x28
    buf[32] = 0x28;
    buf[39] = 0x00;
    buf[40] = 0x08;
    // Immediate data segment "abcde"
    buf[48] = 'a';
    buf[49] = 'b';
    buf[50] = 'c';
    buf[51] = 'd';
    buf[52] = 'e';

    const pdu = parseIscsiPdu(buf[0..56], false, false) ?? unreachable;
    expect_eq(pdu.consumed, 56);
    expectBytesEq(pdu.data_segment, "abcde");

    const cmd = parseIscsiScsiCmd(pdu) ?? unreachable;
    expect_eq(cmd.read_flag, true);
    expect_eq(cmd.write_flag, false);
    expect_eq(cmd.attr, 1 as u8);
    expect_eq(cmd.expected_data_len, 4096 as u32);
    expect_eq(cmd.cmd_sn, 100 as u32);
    expect_eq(cmd.exp_stat_sn, 50 as u32);
    expect_eq(cmd.cdb[0], 0x28 as u8);
    expect_eq(cmd.cdb[8], 0x08 as u8);
});

test("4. HeaderDigest & DataDigest dual Castagnoli CRC32C verification with AHS", () => {
    // 48B BHS + 4B AHS (1 word) + 4B HeaderDigest + 8B padded data (6B actual + 2B pad) + 4B DataDigest = 68 bytes
    let buf: [68]u8 = [0; 68];
    buf[0] = OP_DATA_IN;
    buf[1] = 0x80;
    buf[4] = 1; // 1 AHS word = 4 bytes
    buf[7] = 6; // 6 bytes data -> padded to 8 bytes
    // AHS at 48..52
    buf[48] = 0x00;
    buf[49] = 0x01;
    buf[50] = 0x02;
    buf[51] = 0x03;
    // HeaderDigest at 52..56 covers BHS+AHS (0..52)
    const hcrc = iscsiCrc32c(buf[0..52]);
    writeU32LeTest(buf[0..], 52, hcrc);
    // DataSegment at 56..64 ("123456" + 2 zero pad)
    buf[56] = '1';
    buf[57] = '2';
    buf[58] = '3';
    buf[59] = '4';
    buf[60] = '5';
    buf[61] = '6';
    // DataDigest at 64..68 covers padded data (56..64)
    const dcrc = iscsiCrc32c(buf[56..64]);
    writeU32LeTest(buf[0..], 64, dcrc);

    const pdu = parseIscsiPdu(buf[0..], true, true) ?? unreachable;
    expect_eq(pdu.ahs.len, 4);
    expectBytesEq(pdu.data_segment, "123456");
    expect_eq(pdu.consumed, 68);

    // Flip 1 bit in HeaderDigest -> rejected
    let bad_hdr: [68]u8 = buf;
    bad_hdr[52] = bad_hdr[52] ^ 0x01;
    expect_eq(parseIscsiPdu(bad_hdr[0..], true, true) == null, true);

    // Flip 1 bit in DataDigest -> rejected
    let bad_dat: [68]u8 = buf;
    bad_dat[67] = bad_dat[67] ^ 0x80;
    expect_eq(parseIscsiPdu(bad_dat[0..], true, true) == null, true);
});

test("5. Truncated BHS, AHS overflow, and truncated DataSegment rejection", () => {
    let short_bhs: [47]u8 = [0; 47];
    expect_eq(parseIscsiPdu(short_bhs[0..], false, false) == null, true);

    // AHS claims 4 words (16B) but buffer is only 48B
    let bad_ahs: [48]u8 = [0; 48];
    bad_ahs[4] = 4;
    expect_eq(parseIscsiPdu(bad_ahs[0..], false, false) == null, true);

    // DataSegment claims 10 bytes (12B padded) but only 4 bytes follow
    let bad_data: [52]u8 = [0; 52];
    bad_data[7] = 10;
    expect_eq(parseIscsiPdu(bad_data[0..], false, false) == null, true);

    // Missing HeaderDigest when enabled
    let no_hd: [48]u8 = [0; 48];
    expect_eq(parseIscsiPdu(no_hd[0..], true, false) == null, true);
});
