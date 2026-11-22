// Source: cp/cap/test/test_rtcp_production_deep.cp (Production deep tests for cap:rtcp)

import { test, expect_eq } from "cap:test";
import {
    RTCP_VERSION,
    RTCP_HEADER_SIZE,
    RTCP_REPORT_BLOCK_SIZE,
    RTCP_SR_SENDER_INFO_SIZE,
    PT_SR,
    PT_RR,
    PT_SDES,
    PT_BYE,
    PT_APP,
    SDES_END,
    SDES_CNAME,
    SDES_TOOL,
    RtcpHeader,
    RtcpPacket,
    RtcpCompoundIter,
    RtcpReportBlock,
    RtcpReportBlockIter,
    RtcpSr,
    RtcpRr,
    RtcpByePacket as RtcpBye,
    RtcpApp,
    RtcpSdesItem,
    RtcpSdesItemIter,
    RtcpSdesChunk,
    isRtcp,
    parseRtcpPacket,
    iterRtcpCompound,
    parseRtcpSr,
    parseRtcpRr,
    parseRtcpBye,
    parseRtcpApp,
} from "cap:net";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn writeBeU16(buf: *[]u8, off: usize, v: u16) {
    buf.*[off] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 3] = (v & 0xFF) as u8;
}

test("1. Compound RTCP packet (SR + RR + BYE) zero-copy iteration & 24-bit signed cumulative_lost", () => {
    expect_eq(RTCP_VERSION, 2);
    expect_eq(RTCP_HEADER_SIZE, 4);
    expect_eq(RTCP_REPORT_BLOCK_SIZE, 24);
    expect_eq(RTCP_SR_SENDER_INFO_SIZE, 20);

    // 拼接 3 个 RTCP 包组成 Compound Packet:
    // 1) SR (RC=1): 4B hdr + 4B ssrc + 20B sender_info + 24B report = 52B (length_words = 12) @ 0..52
    // 2) RR (RC=1): 4B hdr + 4B ssrc + 24B report = 32B (length_words = 7) @ 52..84
    // 3) BYE (SC=1 + reason "done"): 4B hdr + 4B ssrc + 1B len(4) + 4B "done" + 3B zero = 16B (length_words = 3) @ 84..100
    let buf: [100]u8 = [0; 100];
    let s: []u8 = buf[0..100];

    // 1) SR @ 0
    s[0] = 0x81; // V=2, P=0, RC=1
    s[1] = PT_SR;
    writeBeU16(*s, 2, 12);
    writeBeU32(*s, 4, 0x11223344);  // sender SSRC
    writeBeU32(*s, 8, 3900000000);  // ntp_sec
    writeBeU32(*s, 12, 0x80000000); // ntp_frac
    writeBeU32(*s, 16, 480000);     // rtp_ts
    writeBeU32(*s, 20, 1500);       // packet_count
    writeBeU32(*s, 24, 1800000);    // octet_count
    // Report Block 0 @ 28..52 (negative cumulative_lost = -5 -> 24-bit 0xFFFFFB)
    writeBeU32(*s, 28, 0xAABBCCDD);
    s[32] = 12; // fraction_lost
    s[33] = 0xFF; s[34] = 0xFF; s[35] = 0xFB; // cumulative_lost = -5
    writeBeU32(*s, 36, 0x00010020); // highest_seq
    writeBeU32(*s, 40, 250);        // jitter
    writeBeU32(*s, 44, 0x12345678); // lsr
    writeBeU32(*s, 48, 65536);      // dlsr

    // 2) RR @ 52
    s[52] = 0x81; // V=2, P=0, RC=1
    s[53] = PT_RR;
    writeBeU16(*s, 54, 7);
    writeBeU32(*s, 56, 0x55667788);
    // Report Block 0 @ 60..84 (positive cumulative_lost = 42)
    writeBeU32(*s, 60, 0x11223344);
    s[64] = 0;
    s[65] = 0x00; s[66] = 0x00; s[67] = 42;
    writeBeU32(*s, 68, 999);
    writeBeU32(*s, 72, 15);

    // 3) BYE @ 84
    s[84] = 0x81; // V=2, P=0, SC=1
    s[85] = PT_BYE;
    writeBeU16(*s, 86, 3);
    writeBeU32(*s, 88, 0x11223344);
    s[92] = 4; // reason len = 4
    s[93] = 100; s[94] = 111; s[95] = 110; s[96] = 101; // "done"

    expect_eq(isRtcp(buf[0..100]), true);
    let it = iterRtcpCompound(buf[0..100]);

    // Packet 1: SR
    const p1 = it.next() ?? unreachable;
    expect_eq(p1.header.byteLen(), 52);
    const sr = parseRtcpSr(p1) ?? unreachable;
    expect_eq(sr.ssrc, 0x11223344);
    expect_eq(sr.ntp_sec, 3900000000);
    expect_eq(sr.packet_count, 1500);
    expect_eq(sr.octet_count, 1800000);
    let rbit1 = sr.reports();
    const rb1 = rbit1.next() ?? unreachable;
    expect_eq(rb1.ssrc, 0xAABBCCDD);
    expect_eq(rb1.fraction_lost, 12);
    expect_eq(rb1.cumulative_lost, -5);
    expect_eq(rb1.jitter, 250);
    expect_eq(rbit1.next() == null, true);

    // Packet 2: RR
    const p2 = it.next() ?? unreachable;
    const rr = parseRtcpRr(p2) ?? unreachable;
    expect_eq(rr.ssrc, 0x55667788);
    let rbit2 = rr.reports();
    const rb2 = rbit2.next() ?? unreachable;
    expect_eq(rb2.cumulative_lost, 42);
    expect_eq(rb2.highest_seq, 999);
    expect_eq(rbit2.next() == null, true);

    // Packet 3: BYE
    const p3 = it.next() ?? unreachable;
    const bye = parseRtcpBye(p3) ?? unreachable;
    expect_eq(bye.src_count, 1);
    expect_eq(bye.ssrcAt(0) ?? 0, 0x11223344);
    expect_eq(bye.ssrcAt(1) == null, true);
    expect_eq(sliceEq(bye.reason, "done"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. RTCP Padding bit stripping & APP (PT=204) parsing", () => {
    // APP packet with P=1, subtype=3, name="TEST", data="hi" (2B) + 2B padding (pad_len=2)
    // Total = 4B hdr + 4B ssrc + 4B name + 2B data + 2B pad = 16B (length_words = 3)
    let buf: [16]u8 = [0; 16];
    let s: []u8 = buf[0..16];
    s[0] = 0xA3; // V=2 (0x80) | P=1 (0x20) | subtype=3
    s[1] = PT_APP;
    writeBeU16(*s, 2, 3);
    writeBeU32(*s, 4, 0xCAFEBABE);
    s[8] = 84; s[9] = 69; s[10] = 83; s[11] = 84; // "TEST"
    s[12] = 104; s[13] = 105; // "hi"
    s[14] = 0; s[15] = 2;     // padding = 2 bytes

    const pkt = parseRtcpPacket(buf[0..16]) ?? unreachable;
    expect_eq(pkt.header.padding, true);
    expect_eq(pkt.body.len, 10); // 12 - 2 padding = 10

    const app = parseRtcpApp(pkt) ?? unreachable;
    expect_eq(app.subtype, 3);
    expect_eq(app.ssrc, 0xCAFEBABE);
    expect_eq(app.name[0], 84);
    expect_eq(app.name[3], 84);
    expect_eq(sliceEq(app.data, "hi"), true);
});

test("3. SDES Chunk & Item iteration (CNAME + TOOL + END)", () => {
    const items_raw: [14]u8 = [
        SDES_CNAME, 4, 117, 115, 114, 49, // "usr1"
        SDES_TOOL, 5, 99, 112, 114, 116, 112, // "cprtp"
        SDES_END,
    ];
    const chunk = RtcpSdesChunk {
        ssrc: 0x01020304,
        items_bytes: items_raw[0..14],
    };
    let it = chunk.items();
    const i1 = it.next() ?? unreachable;
    expect_eq(i1.item_type, SDES_CNAME);
    expect_eq(sliceEq(i1.value, "usr1"), true);

    const i2 = it.next() ?? unreachable;
    expect_eq(i2.item_type, SDES_TOOL);
    expect_eq(sliceEq(i2.value, "cprtp"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("4. BYE with multiple SSRCs and no reason", () => {
    // 4B hdr + 2 * 4B SSRCs = 12B (length_words = 2)
    let buf: [12]u8 = [0; 12];
    let s: []u8 = buf[0..12];
    s[0] = 0x82; // V=2, SC=2
    s[1] = PT_BYE;
    writeBeU16(*s, 2, 2);
    writeBeU32(*s, 4, 0x0000000A);
    writeBeU32(*s, 8, 0x0000000B);

    const pkt = parseRtcpPacket(buf[0..12]) ?? unreachable;
    const bye = parseRtcpBye(pkt) ?? unreachable;
    expect_eq(bye.src_count, 2);
    expect_eq(bye.ssrcAt(0) ?? 0, 0x0A);
    expect_eq(bye.ssrcAt(1) ?? 0, 0x0B);
    expect_eq(bye.reason.len, 0);
});

test("5. Malformed RTCP defense: bad version, truncated length, out-of-bounds padding, corrupt SDES", () => {
    // 5.1 短于 4B
    const short3: [3]u8 = [0x80, 200, 0];
    expect_eq(isRtcp(short3[0..3]), false);
    expect_eq(parseRtcpPacket(short3[0..3]) == null, true);

    // 5.2 非法版本 (V=1 -> 0x40)
    const bad_ver: [8]u8 = [0x40, 201, 0, 1, 0, 0, 0, 0];
    expect_eq(isRtcp(bad_ver[0..8]), false);
    expect_eq(parseRtcpPacket(bad_ver[0..8]) == null, true);

    // 5.3 length_words 声明 12B 但仅给 8B
    const trunc: [8]u8 = [0x80, 201, 0, 2, 0, 0, 0, 0];
    expect_eq(isRtcp(trunc[0..8]), false);
    expect_eq(parseRtcpPacket(trunc[0..8]) == null, true);

    // 5.4 Padding=1 但 pad_len=0 或 pad_len > body_len 均必须拒绝
    const pad_zero: [8]u8 = [0xA0, 201, 0, 1, 0, 0, 0, 0];
    expect_eq(parseRtcpPacket(pad_zero[0..8]) == null, true);
    const pad_oob: [8]u8 = [0xA0, 201, 0, 1, 0, 0, 0, 5]; // body only 4B, pad=5
    expect_eq(parseRtcpPacket(pad_oob[0..8]) == null, true);

    // 5.5 复合包第 2 个子包截断 -> CompoundIter 置 had_error = true
    const bad_comp: [12]u8 = [
        0x80, 201, 0, 1, 0, 0, 0, 1, // valid 8B RR
        0x80, 203, 0, 2,             // truncated BYE (claims 12B)
    ];
    let cit = iterRtcpCompound(bad_comp[0..12]);
    const first = cit.next() ?? unreachable;
    expect_eq(first.header.pt, PT_RR);
    expect_eq(cit.next() == null, true);
    expect_eq(cit.had_error, true);

    // 5.6 截断 SDES Item 置 had_error = true
    const bad_sdes: [3]u8 = [SDES_CNAME, 10, 65];
    let sit = RtcpSdesItemIter.init(bad_sdes[0..3]);
    expect_eq(sit.next() == null, true);
    expect_eq(sit.had_error, true);
});
