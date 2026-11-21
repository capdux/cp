// Source: cp/cap/test/test_dnp3_production_deep.cp (Production deep tests for cap:dnp3)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DNP3_TCP_PORT,
    DNP3_TLS_PORT,
    DNP3_START_BYTES,
    DNP3_HEADER_SIZE,
    DNP3_BLOCK_SIZE,
    DNP3_MAX_PAYLOAD_LEN,
    DNP3_CRC_POLY,
    DL_DIR,
    DL_PRM,
    DL_FCB_DFC,
    DL_FCV,
    FC_CONFIRM,
    FC_READ,
    FC_WRITE,
    FC_SELECT,
    FC_OPERATE,
    FC_DIRECT_OPERATE,
    FC_COLD_RESTART,
    FC_RESPONSE,
    FC_UNSOLICITED_RESPONSE,
    dnp3Crc16,
    Dnp3LinkHeader,
    Dnp3TransportHeader,
    Dnp3AppHeader,
    isDnp3,
    parseDnp3LinkHeader,
    dnp3WireFrameSize,
    extractDnp3Payload,
    encodeDnp3Frame,
    parseDnp3Transport,
    encodeDnp3Transport,
    parseDnp3App,
} from "cap:dnp3";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. IEEE 1815 standard DNP3 CRC-16 test vectors and protocol constants", () => {
    expect_eq(DNP3_TCP_PORT, 20000 as u16);
    expect_eq(DNP3_TLS_PORT, 19999 as u16);
    expect_eq(DNP3_START_BYTES, 0x0564 as u16);
    expect_eq(DNP3_HEADER_SIZE, 10);
    expect_eq(DNP3_BLOCK_SIZE, 16);
    expect_eq(DNP3_CRC_POLY, 0xA6BC as u16);

    // Official CRC-16/DNP check value for ASCII "123456789" is 0xEA82
    expect_eq(dnp3Crc16("123456789"), 0xEA82 as u16);

    // Empty slice CRC-16/DNP = 0x0000 ^ 0xFFFF = 0xFFFF
    expect_eq(dnp3Crc16(""), 0xFFFF as u16);

    // Classic IEEE 1815 Link Header example: Reset Link States [05 64 05 C0 01 00 00 04]
    const reset_hdr: [8]u8 = [0x05, 0x64, 0x05, 0xC0, 0x01, 0x00, 0x00, 0x04];
    const crc = dnp3Crc16(reset_hdr[0..]);
    expect_eq(crc, 0xE921 as u16);
});

test("2. 10B Link Header and multi-16B-block CRC encode/extract round-trip", () => {
    // 35-byte payload -> 3 blocks (16B + 16B + 3B) => wire size = 10 + 35 + 3*2 = 51 bytes
    let payload: [35]u8 = [0; 35];
    let i: usize = 0;
    while (i < 35) {
        payload[i] = ((i * 7 + 3) & 0xFF) as u8;
        i += 1;
    }

    expect_eq(dnp3WireFrameSize(35) ?? 0, 51);
    expect_eq(dnp3WireFrameSize(0) ?? 0, 10);
    expect_eq(dnp3WireFrameSize(251) == null, true);

    let wire: [64]u8 = [0; 64];
    const ctrl: u8 = DL_DIR | DL_PRM | 0x04; // Unconfirmed User Data from Master
    const wire_len = encodeDnp3Frame(ctrl, 0x000A, 0x0001, payload[0..], wire[0..]) ?? unreachable;
    expect_eq(wire_len, 51);
    expect_eq(isDnp3(wire[0..wire_len]), true);

    const hdr = parseDnp3LinkHeader(wire[0..wire_len]) ?? unreachable;
    expect_eq(hdr.length, 40 as u8); // 5 + 35
    expect_eq(hdr.isFromMaster(), true);
    expect_eq(hdr.isPrimary(), true);
    expect_eq(hdr.funcCode(), 4 as u8);
    expect_eq(hdr.dst, 10 as u16);
    expect_eq(hdr.src, 1 as u16);
    expect_eq(hdr.payloadLen(), 35);

    let extracted: [64]u8 = [0; 64];
    const ext_len = extractDnp3Payload(wire[0..wire_len], extracted[0..]) ?? unreachable;
    expect_eq(ext_len, 35);
    expectBytesEq(extracted[0..ext_len], payload[0..]);

    // Zero-payload frame round-trip (10 bytes total)
    const empty_len = encodeDnp3Frame(DL_DIR | DL_PRM, 1, 2, "", wire[0..]) ?? unreachable;
    expect_eq(empty_len, 10);
    expect_eq(extractDnp3Payload(wire[0..empty_len], extracted[0..]) ?? 99, 0);
});

test("3. Transport Layer header (FIN/FIR/SEQ) encode & decode", () => {
    const b1 = encodeDnp3Transport(true, true, 42);
    expect_eq(b1, 0xEA as u8);
    const t1 = parseDnp3Transport(b1);
    expect_eq(t1.fin, true);
    expect_eq(t1.fir, true);
    expect_eq(t1.seq, 42 as u8);

    const b2 = encodeDnp3Transport(false, true, 0);
    const t2 = parseDnp3Transport(b2);
    expect_eq(t2.fin, false);
    expect_eq(t2.fir, true);
    expect_eq(t2.seq, 0 as u8);

    const t3 = parseDnp3Transport(0xBF); // FIN=1, FIR=0, SEQ=63
    expect_eq(t3.fin, true);
    expect_eq(t3.fir, false);
    expect_eq(t3.seq, 63 as u8);
});

test("4. Application Request (2B header) and Response / Unsolicited (4B header with 2B IIN)", () => {
    // Master READ Request: AC=0xC3 (FIR=1, FIN=1, CON=0, UNS=0, SEQ=3), FC=1 (READ), Objects=[60, 1, 6]
    const req_frag: [5]u8 = [0xC3, FC_READ, 60, 1, 6];
    const req = parseDnp3App(req_frag[0..]) ?? unreachable;
    expect_eq(req.fir, true);
    expect_eq(req.fin, true);
    expect_eq(req.con, false);
    expect_eq(req.uns, false);
    expect_eq(req.seq, 3 as u8);
    expect_eq(req.func_code, FC_READ);
    expect_eq(req.isResponse(), false);
    expect_eq(req.iin == null, true);
    expect_eq(req.objects.len, 3);
    expect_eq(req.objects[0], 60 as u8);

    // Outstation RESPONSE: AC=0xE5 (FIR=1, FIN=1, CON=1, UNS=0, SEQ=5), FC=129, IIN=0x0080 (LE: 0x80, 0x00), Obj=[1, 2, 0]
    const resp_frag: [7]u8 = [0xE5, FC_RESPONSE, 0x80, 0x00, 1, 2, 0];
    const resp = parseDnp3App(resp_frag[0..]) ?? unreachable;
    expect_eq(resp.fir, true);
    expect_eq(resp.fin, true);
    expect_eq(resp.con, true);
    expect_eq(resp.uns, false);
    expect_eq(resp.seq, 5 as u8);
    expect_eq(resp.func_code, FC_RESPONSE);
    expect_eq(resp.isResponse(), true);
    expect_eq(resp.iin ?? 0, 0x0080 as u16);
    expect_eq(resp.objects.len, 3);

    // Unsolicited Response: AC=0xF7 (FIR=1, FIN=1, CON=1, UNS=1, SEQ=7), FC=130, IIN=0x0102
    const uns_frag: [4]u8 = [0xF7, FC_UNSOLICITED_RESPONSE, 0x02, 0x01];
    const uns = parseDnp3App(uns_frag[0..]) ?? unreachable;
    expect_eq(uns.uns, true);
    expect_eq(uns.iin ?? 0, 0x0102 as u16);
    expect_eq(uns.objects.len, 0);

    // Truncated app fragments rejected
    expect_eq(parseDnp3App(req_frag[0..1]) == null, true);
    expect_eq(parseDnp3App(resp_frag[0..3]) == null, true);
});

test("5. Malformed DNP3 rejection (bad start bytes, length<5, bad header CRC, bad block CRC, truncation)", () => {
    let wire: [64]u8 = [0; 64];
    const sample = "DNP3-payload-exceeding-16-bytes!";
    const n = encodeDnp3Frame(DL_DIR | DL_PRM | 4, 2, 1, sample, wire[0..]) ?? unreachable;
    let out: [64]u8 = [0; 64];

    // 1. Corrupt start byte
    let bad_start: [64]u8 = wire;
    bad_start[0] = 0x06;
    expect_eq(isDnp3(bad_start[0..n]), false);
    expect_eq(parseDnp3LinkHeader(bad_start[0..n]) == null, true);

    // 2. Corrupt header CRC
    let bad_hcrc: [64]u8 = wire;
    bad_hcrc[9] = bad_hcrc[9] ^ 0x01;
    expect_eq(isDnp3(bad_hcrc[0..n]), false);
    expect_eq(extractDnp3Payload(bad_hcrc[0..n], out[0..]) == null, true);

    // 3. length < 5 even with valid CRC
    let bad_len: [10]u8 = [0x05, 0x64, 0x04, 0xC0, 0x01, 0x00, 0x02, 0x00, 0, 0];
    const c_bad = dnp3Crc16(bad_len[0..8]);
    bad_len[8] = (c_bad & 0xFF) as u8;
    bad_len[9] = ((c_bad >> 8) & 0xFF) as u8;
    expect_eq(isDnp3(bad_len[0..]), false);

    // 4. Corrupt payload block CRC (flip bit in first data block)
    let bad_blk: [64]u8 = wire;
    bad_blk[12] = bad_blk[12] ^ 0x80;
    expect_eq(isDnp3(bad_blk[0..n]), true); // header still valid
    expect_eq(extractDnp3Payload(bad_blk[0..n], out[0..]) == null, true); // block CRC fails!

    // 5. Truncated frame or small output buffer
    expect_eq(extractDnp3Payload(wire[0..n - 1], out[0..]) == null, true);
    let tiny: [8]u8 = [0; 8];
    expect_eq(extractDnp3Payload(wire[0..n], tiny[0..]) == null, true);
    expect_eq(encodeDnp3Frame(0xC4, 1, 2, sample, tiny[0..]) == null, true);
});
