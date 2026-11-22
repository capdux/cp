// Source: cp/cap/test/test_ts_production_deep.cp (Production deep tests for cap:ts)

import { test, expect_eq } from "cap:test";
import {
    TS_PACKET_SIZE,
    M2TS_PACKET_SIZE,
    TS_SYNC_BYTE,
    TS_PID_PAT,
    TS_PID_NULL,
    STREAM_TYPE_AAC_ADTS,
    STREAM_TYPE_H264,
    STREAM_TYPE_HEVC,
    TsPacket,
    TsPacketIter,
    PatEntry,
    PatIter,
    PmtStream,
    PmtHeader,
    PmtStreamIter,
    isMpegTs,
    parseTsPacket,
    parsePmt,
} from "cap:binfmt";

fn writeTsHeader(
    buf: *[]u8,
    off: usize,
    pusi: bool,
    pid: u16,
    has_adapt: bool,
    has_payload: bool,
    cc: u8,
) {
    buf[off] = TS_SYNC_BYTE;
    const pusi_bit: u8 = if (pusi) 0x40 else 0x00;
    buf[off + 1] = (((pid >> 8) & 0x1F) as u8) | pusi_bit;
    buf[off + 2] = (pid & 0xFF) as u8;
    const adapt_bit: u8 = if (has_adapt) 0x20 else 0x00;
    const pay_bit: u8 = if (has_payload) 0x10 else 0x00;
    buf[off + 3] = (cc & 0x0F) | adapt_bit | pay_bit;
}

test("1. isMpegTs and TsPacketIter handle multi-packet 188B TS and 192B M2TS streams", () => {
    // 2 standard 188B TS packets = 376 bytes
    let stream: [376]u8 = [0xFF; 376];
    let s = stream[0..376];
    writeTsHeader(*s, 0, true, TS_PID_PAT, false, true, 0);
    writeTsHeader(*s, 188, false, 0x0100, false, true, 1);

    expect_eq(isMpegTs(s), true);

    let it = TsPacketIter.init(s);
    const p0 = it.next() ?? unreachable;
    expect_eq(p0.pid, TS_PID_PAT);
    expect_eq(p0.payload_unit_start, true);
    expect_eq(p0.continuity_counter, 0);
    expect_eq(p0.payload.len, 184);

    const p1 = it.next() ?? unreachable;
    expect_eq(p1.pid, 0x0100);
    expect_eq(p1.payload_unit_start, false);
    expect_eq(p1.continuity_counter, 1);
    expect_eq(p1.payload.len, 184);

    expect_eq(it.next() == null, true);

    // 2 M2TS packets (4B arrival timestamp + 188B TS) = 384 bytes
    let m2ts: [384]u8 = [0; 384];
    let m = m2ts[0..384];
    writeTsHeader(*m, 4, true, 0x0101, false, true, 3);
    writeTsHeader(*m, 196, false, TS_PID_NULL, false, false, 4);

    expect_eq(isMpegTs(m), true);

    let mit = TsPacketIter.init(m);
    const mp0 = mit.next() ?? unreachable;
    expect_eq(mp0.pid, 0x0101);
    expect_eq(mp0.continuity_counter, 3);
    const mp1 = mit.next() ?? unreachable;
    expect_eq(mp1.pid, TS_PID_NULL);
    expect_eq(mp1.has_payload, false);
    expect_eq(mp1.payload.len, 0);
    expect_eq(mit.next() == null, true);
});

test("2. parseTsPacket decodes Adaptation Field flags and 33-bit 90kHz PCR to milliseconds", () => {
    let pkt: [188]u8 = [0xAA; 188];
    let b = pkt[0..188];
    writeTsHeader(*b, 0, false, 0x0100, true, true, 7);

    // Adaptation field: length = 7 (1B flags + 6B PCR)
    b[4] = 7;
    b[5] = 0x80 | 0x40 | 0x10; // discontinuity=1, random_access=1, pcr_flag=1

    // Encode pcr_base = 900000 (10000 ms = 10s), pcr_ext = 257 (0x101)
    const pcr_base: u64 = 900000;
    const pcr_ext: u16 = 257;
    b[6] = ((pcr_base >> 25) & 0xFF) as u8;
    b[7] = ((pcr_base >> 17) & 0xFF) as u8;
    b[8] = ((pcr_base >> 9) & 0xFF) as u8;
    b[9] = ((pcr_base >> 1) & 0xFF) as u8;
    b[10] = (((pcr_base & 0x01) << 7) as u8) | 0x7E | (((pcr_ext >> 8) & 0x01) as u8);
    b[11] = (pcr_ext & 0xFF) as u8;

    const parsed = parseTsPacket(b) ?? unreachable;
    expect_eq(parsed.pid, 0x0100);
    expect_eq(parsed.has_adaptation, true);
    expect_eq(parsed.discontinuity, true);
    expect_eq(parsed.random_access, true);
    expect_eq(parsed.pcr_base ?? 0, 900000);
    expect_eq(parsed.pcr_ext ?? 0, 257);
    expect_eq(parsed.pcr_ms() ?? 0, 10000);
    // payload starts at 4 + 1 + 7 = 12, length = 188 - 12 = 176
    expect_eq(parsed.payload.len, 176);
});

test("3. PatIter skips pointer_field and extracts all program-to-PMT-PID mappings", () => {
    let pkt: [188]u8 = [0xFF; 188];
    let b = pkt[0..188];
    writeTsHeader(*b, 0, true, TS_PID_PAT, false, true, 0);

    // Payload starts at offset 4:
    // pointer_field = 0
    b[4] = 0x00;
    // table_id = 0x00 (PAT)
    b[5] = 0x00;
    // section_length = 5 (ext hdr) + 8 (2 entries) + 4 (CRC) = 17 (0x011)
    b[6] = 0xB0;
    b[7] = 17;
    // transport_stream_id = 0x0001
    b[8] = 0x00;
    b[9] = 0x01;
    // version=0, current_next=1
    b[10] = 0xC1;
    b[11] = 0x00; // section_number
    b[12] = 0x00; // last_section_number

    // Entry 0: program_number = 0 (NIT), pid = 0x0010
    b[13] = 0x00;
    b[14] = 0x00;
    b[15] = 0xE0;
    b[16] = 0x10;

    // Entry 1: program_number = 1, pmt_pid = 0x1000
    b[17] = 0x00;
    b[18] = 0x01;
    b[19] = 0xF0;
    b[20] = 0x00;

    const ts = parseTsPacket(b) ?? unreachable;
    let pat = PatIter.init(ts.payload);

    const e0 = pat.next() ?? unreachable;
    expect_eq(e0.program_number, 0);
    expect_eq(e0.pmt_pid, 0x0010);

    const e1 = pat.next() ?? unreachable;
    expect_eq(e1.program_number, 1);
    expect_eq(e1.pmt_pid, 0x1000);

    expect_eq(pat.next() == null, true);
});

test("4. parsePmt and PmtStreamIter extract PCR PID and H.264/AAC/HEVC elementary streams", () => {
    let pkt: [188]u8 = [0xFF; 188];
    let b = pkt[0..188];
    writeTsHeader(*b, 0, true, 0x1000, false, true, 0);

    // pointer_field = 0
    b[4] = 0x00;
    // table_id = 0x02 (PMT)
    b[5] = 0x02;
    // section_length: 9 (ext hdr) + 2 (program_info) + (5 + 2) (H264+desc) + 5 (AAC) + 5 (HEVC) + 4 (CRC) = 32
    b[6] = 0xB0;
    b[7] = 32;
    // program_number = 1
    b[8] = 0x00;
    b[9] = 0x01;
    // version = 2, current_next = 1 => 0xC0 | (2 << 1) | 1 = 0xC5
    b[10] = 0xC5;
    b[11] = 0x00;
    b[12] = 0x00;
    // pcr_pid = 0x0100
    b[13] = 0xE1;
    b[14] = 0x00;
    // program_info_len = 2
    b[15] = 0xF0;
    b[16] = 0x02;
    // 2 bytes program_info descriptors
    b[17] = 0x05;
    b[18] = 0x00;

    // Stream 0: H.264 (0x1B), pid = 0x0100, es_info_len = 2
    b[19] = STREAM_TYPE_H264;
    b[20] = 0xE1;
    b[21] = 0x00;
    b[22] = 0xF0;
    b[23] = 0x02;
    b[24] = 0x28;
    b[25] = 0x00;

    // Stream 1: AAC ADTS (0x0F), pid = 0x0101, es_info_len = 0
    b[26] = STREAM_TYPE_AAC_ADTS;
    b[27] = 0xE1;
    b[28] = 0x01;
    b[29] = 0xF0;
    b[30] = 0x00;

    // Stream 2: HEVC (0x24), pid = 0x0102, es_info_len = 0
    b[31] = STREAM_TYPE_HEVC;
    b[32] = 0xE1;
    b[33] = 0x02;
    b[34] = 0xF0;
    b[35] = 0x00;

    const ts = parseTsPacket(b) ?? unreachable;
    const pmt = parsePmt(ts.payload) ?? unreachable;
    expect_eq(pmt.program_number, 1);
    expect_eq(pmt.version_number, 2);
    expect_eq(pmt.current_next, true);
    expect_eq(pmt.pcr_pid, 0x0100);
    expect_eq(pmt.program_info_len, 2);

    let sit = pmt.streams();
    const s0 = sit.next() ?? unreachable;
    expect_eq(s0.stream_type, STREAM_TYPE_H264);
    expect_eq(s0.elementary_pid, 0x0100);
    expect_eq(s0.es_info_len, 2);

    const s1 = sit.next() ?? unreachable;
    expect_eq(s1.stream_type, STREAM_TYPE_AAC_ADTS);
    expect_eq(s1.elementary_pid, 0x0101);
    expect_eq(s1.es_info_len, 0);

    const s2 = sit.next() ?? unreachable;
    expect_eq(s2.stream_type, STREAM_TYPE_HEVC);
    expect_eq(s2.elementary_pid, 0x0102);

    expect_eq(sit.next() == null, true);
});

test("5. truncated packets, bad sync byte, out-of-bounds adaptation_length and section_length fail safely", () => {
    const empty: [0]u8 = [];
    expect_eq(isMpegTs(empty[0..0]), false);
    expect_eq(parseTsPacket(empty[0..0]) == null, true);
    expect_eq(parsePmt(empty[0..0]) == null, true);

    let pkt: [188]u8 = [0; 188];
    let b = pkt[0..188];
    writeTsHeader(*b, 0, true, 0x0100, true, true, 0);

    // Truncated packet (< 188 bytes)
    expect_eq(parseTsPacket(b[0..187]) == null, true);
    expect_eq(isMpegTs(b[0..187]), false);

    // Bad sync byte
    b[0] = 0x48;
    expect_eq(parseTsPacket(b) == null, true);
    expect_eq(isMpegTs(b), false);
    b[0] = TS_SYNC_BYTE;

    // Out-of-bounds adaptation_length (> 183)
    b[4] = 184;
    expect_eq(parseTsPacket(b) == null, true);

    // PCR flag set but adaptation_length < 7
    b[4] = 5;
    b[5] = 0x10; // pcr_flag=1
    expect_eq(parseTsPacket(b) == null, true);

    // PAT with wrong table_id or section_length overflowing payload
    let bad_psi: [24]u8 = [0; 24];
    let bp = bad_psi[0..24];
    bp[0] = 0; // pointer_field
    bp[1] = 0x00; // PAT
    bp[2] = 0xB0;
    bp[3] = 200; // section_length = 200 > 24
    let bad_pat = PatIter.init(bp);
    expect_eq(bad_pat.next() == null, true);

    // PMT with wrong table_id (0x01 instead of 0x02)
    bp[1] = 0x01;
    bp[3] = 16;
    expect_eq(parsePmt(bp) == null, true);

    // PMT with program_info_len exceeding section body
    bp[1] = 0x02;
    bp[3] = 16; // valid total section_length
    bp[11] = 0xF0;
    bp[12] = 50; // program_info_len = 50 > remaining
    expect_eq(parsePmt(bp) == null, true);
});
