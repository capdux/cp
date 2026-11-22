// Source: cp/cap/test/test_srec_production_deep.cp
// Deep Production Test Suite for cap:srec
// Covers:
//   1. Full S0 header + S1(16-bit)/S2(24-bit)/S3(32-bit) data + S5/S6 count + S7/S8/S9 termination parsing
//   2. encodeSrecLine -> parseSrecLine round-trip across 16/24/32-bit address records
//   3. summarizeSrec single-pass address range, total_bytes, entry_addr & S5/S6 count matching/mismatching
//   4. Reserved S4 rejection, one's-complement checksum mismatch, non-zero payload on S5-S9 rejection
//   5. Truncated lines, invalid hex, wrong line length, address overflow & small output buffer guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SrecType,
    SrecRecord,
    SrecStats,
    SrecIter,
    srecAddrBytes,
    srecChecksum,
    parseSrecLine,
    encodeSrecLine,
    summarizeSrec,
} from "cap:iot";

fn testFullS0ToS9Parsing() {
    // Address byte widths
    expect_eq(srecAddrBytes(SrecType.s0_header), 2);
    expect_eq(srecAddrBytes(SrecType.s1_data16), 2);
    expect_eq(srecAddrBytes(SrecType.s2_data24), 3);
    expect_eq(srecAddrBytes(SrecType.s3_data32), 4);
    expect_eq(srecAddrBytes(SrecType.s5_count16), 2);
    expect_eq(srecAddrBytes(SrecType.s6_count24), 3);
    expect_eq(srecAddrBytes(SrecType.s7_start32), 4);
    expect_eq(srecAddrBytes(SrecType.s8_start24), 3);
    expect_eq(srecAddrBytes(SrecType.s9_start16), 2);
    expect_eq(srecAddrBytes(SrecType.unknown), 0);

    // S0 header ("HDR")
    const r0 = parseSrecLine("S00600004844521B\r\n").?;
    expect_eq(r0.rec_type == SrecType.s0_header, true);
    expect_eq(r0.address, 0x0000);
    expect_eq(std.mem.eql(u8, r0.dataSlice(), "HDR"), true);
    expect_eq(r0.isData(), false);
    expect_eq(r0.isTermination(), false);
    expect_eq(r0.checksum, 0x1B);

    // S1 16-bit data
    const r1 = parseSrecLine("S107100001020304DE\n").?;
    expect_eq(r1.rec_type == SrecType.s1_data16, true);
    expect_eq(r1.address, 0x1000);
    expect_eq(r1.data_len, 4);
    expect_eq(r1.dataSlice()[0], 0x01);
    expect_eq(r1.dataSlice()[3], 0x04);
    expect_eq(r1.isData(), true);

    // S2 24-bit data
    const r2 = parseSrecLine("S207012345AABBCC5E").?;
    expect_eq(r2.rec_type == SrecType.s2_data24, true);
    expect_eq(r2.address, 0x012345);
    expect_eq(r2.data_len, 3);
    expect_eq(r2.dataSlice()[0], 0xAA);
    expect_eq(r2.dataSlice()[2], 0xCC);
    expect_eq(r2.isData(), true);

    // S3 32-bit data
    const r3 = parseSrecLine("S30908000000DEADBEEFB6").?;
    expect_eq(r3.rec_type == SrecType.s3_data32, true);
    expect_eq(r3.address, 0x08000000);
    expect_eq(r3.data_len, 4);
    expect_eq(r3.dataSlice()[0], 0xDE);
    expect_eq(r3.dataSlice()[3], 0xEF);
    expect_eq(r3.isData(), true);

    // S5/S6 count & S7/S8/S9 start termination
    const r5 = parseSrecLine("S5030003F9").?;
    expect_eq(r5.rec_type == SrecType.s5_count16, true);
    expect_eq(r5.address, 3);
    expect_eq(r5.data_len, 0);

    const r6 = parseSrecLine("S604000003F8").?;
    expect_eq(r6.rec_type == SrecType.s6_count24, true);
    expect_eq(r6.address, 3);

    const r7 = parseSrecLine("S70508000000F2").?;
    expect_eq(r7.rec_type == SrecType.s7_start32, true);
    expect_eq(r7.address, 0x08000000);
    expect_eq(r7.isTermination(), true);

    const r8 = parseSrecLine("S80401234592").?;
    expect_eq(r8.rec_type == SrecType.s8_start24, true);
    expect_eq(r8.address, 0x012345);
    expect_eq(r8.isTermination(), true);

    const r9 = parseSrecLine("S9031000EC").?;
    expect_eq(r9.rec_type == SrecType.s9_start16, true);
    expect_eq(r9.address, 0x1000);
    expect_eq(r9.isTermination(), true);
}

fn testEncodeParseRoundTrip() {
    let buf: [128]u8 = [0; 128];

    // S1 round-trip
    const p1: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    const n1 = encodeSrecLine(SrecType.s1_data16, 0x1000, p1[0..], buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n1], "S107100001020304DE\n"), true);
    const dec1 = parseSrecLine(buf[0..n1]).?;
    expect_eq(dec1.address, 0x1000);
    expect_eq(std.mem.eql(u8, dec1.dataSlice(), p1[0..]), true);

    // S3 round-trip
    const p3: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    const n3 = encodeSrecLine(SrecType.s3_data32, 0x08000000, p3[0..], buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n3], "S30908000000DEADBEEFB6\n"), true);
    const dec3 = parseSrecLine(buf[0..n3]).?;
    expect_eq(dec3.address, 0x08000000);
    expect_eq(std.mem.eql(u8, dec3.dataSlice(), p3[0..]), true);

    // S7 termination round-trip
    const empty: [0]u8 = [];
    const n7 = encodeSrecLine(SrecType.s7_start32, 0x08000000, empty[0..], buf[0..]).?;
    expect_eq(std.mem.eql(u8, buf[0..n7], "S70508000000F2\n"), true);

    // Direct srecChecksum check
    expect_eq(srecChecksum(0x09, 4, 0x08000000, p3[0..]), 0xB6);
}

fn testSummarizeSrecAndCountValidation() {
    const fw_good =
        "\r\n" ++
        "S00600004844521B\r\n" ++
        "S107100001020304DE\r\n" ++
        "S207012345AABBCC5E\n" ++
        "S30908000000DEADBEEFB6\n" ++
        "S5030003F9\n" ++
        "S70508000000F2\n\n";

    const st = summarizeSrec(fw_good).?;
    expect_eq(st.header_text_len, 3);
    expect_eq(st.data_records, 3);
    expect_eq(st.total_bytes, 11); // 4 + 3 + 4
    expect_eq(st.min_addr, 0x1000);
    expect_eq(st.max_addr, 0x08000003); // 0x08000000 + 4 - 1
    expect_eq(st.entry_addr.?, 0x08000000);
    expect_eq(st.declared_count.?, 3);
    expect_eq(st.count_matches, true);

    // S5 count mismatch (declares 2 instead of 3)
    const fw_mismatch =
        "S00600004844521B\n" ++
        "S107100001020304DE\n" ++
        "S207012345AABBCC5E\n" ++
        "S30908000000DEADBEEFB6\n" ++
        "S5030002FA\n" ++
        "S70508000000F2\n";

    const st_bad = summarizeSrec(fw_mismatch).?;
    expect_eq(st_bad.data_records, 3);
    expect_eq(st_bad.declared_count.?, 2);
    expect_eq(st_bad.count_matches, false);
}

fn testS4AndChecksumRejection() {
    // Reserved S4 rejected
    expect_eq(parseSrecLine("S4030000FC") == null, true);

    // Bad one's-complement checksum (last byte altered DE -> DF)
    expect_eq(parseSrecLine("S107100001020304DF") == null, true);

    // S9 with non-zero payload rejected (even if checksum were valid)
    // S9, count=04, addr=1000, data=AA -> sum=0x04+0x10+0x00+0xAA=0xBE -> ~0xBE=0x41
    expect_eq(parseSrecLine("S9041000AA41") == null, true);

    // S5 with non-zero payload rejected
    expect_eq(parseSrecLine("S5040001AA4B") == null, true);
}

fn testTruncationHexAndEncoderGuards() {
    // Truncated / empty / wrong prefix / non-hex / length mismatch
    expect_eq(parseSrecLine("") == null, true);
    expect_eq(parseSrecLine("S1") == null, true);
    expect_eq(parseSrecLine("X107100001020304DE") == null, true);
    expect_eq(parseSrecLine("S107100001020304") == null, true);     // missing checksum
    expect_eq(parseSrecLine("S107100001020304DE00") == null, true); // extra bytes
    expect_eq(parseSrecLine("S1071000010203ZZDE") == null, true);   // non-hex
    expect_eq(parseSrecLine("S1021000ED") == null, true);           // byte_count < addr_bytes + 1

    // summarizeSrec returns null on malformed firmware line
    const broken_fw = "S00600004844521B\nS107100001020304FF\n";
    expect_eq(summarizeSrec(broken_fw) == null, true);

    // Encoder bounds checks
    let buf: [64]u8 = [0; 64];
    let tiny: [8]u8 = [0; 8];
    const p: [2]u8 = [0x11, 0x22];
    expect_eq(encodeSrecLine(SrecType.unknown, 0, p[0..], buf[0..]) == null, true);
    expect_eq(encodeSrecLine(SrecType.s1_data16, 0x10000, p[0..], buf[0..]) == null, true); // > 16-bit addr
    expect_eq(encodeSrecLine(SrecType.s2_data24, 0x1000000, p[0..], buf[0..]) == null, true); // > 24-bit addr
    expect_eq(encodeSrecLine(SrecType.s9_start16, 0x1000, p[0..], buf[0..]) == null, true); // payload on S9
    expect_eq(encodeSrecLine(SrecType.s1_data16, 0x1000, p[0..], tiny[0..]) == null, true); // buffer too small
}

test("1. Full S0-S9 Record Parsing (16/24/32-bit Data, Count & Termination)", () => {
    testFullS0ToS9Parsing();
});

test("2. encodeSrecLine -> parseSrecLine Round-Trip & Checksum Verification", () => {
    testEncodeParseRoundTrip();
});

test("3. summarizeSrec Address Bounds, Entry Address & S5 Count Matching", () => {
    testSummarizeSrecAndCountValidation();
});

test("4. Reserved S4, One's-Complement Checksum Error & S5-S9 Payload Rejection", () => {
    testS4AndChecksumRejection();
});

test("5. Truncated Lines, Invalid Hex & Encoder Bounds Guards", () => {
    testTruncationHexAndEncoderGuards();
});
