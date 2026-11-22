// Source: cp/cap/test/test_s7_production_deep.cp (Production deep tests for cap:s7)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    S7_TCP_PORT,
    TPKT_VERSION,
    TPKT_HEADER_SIZE,
    S7_PROTOCOL_ID,
    S7_HEADER_SIZE,
    S7_ACK_HEADER_SIZE,
    COTP_CR,
    COTP_CC,
    COTP_DR,
    COTP_DT,
    ROSCTR_JOB,
    ROSCTR_ACK,
    ROSCTR_ACK_DATA,
    ROSCTR_USERDATA,
    FUNC_READ_VAR,
    FUNC_WRITE_VAR,
    FUNC_SETUP_COMM,
    AREA_SYS_INFO,
    AREA_INPUTS,
    AREA_OUTPUTS,
    AREA_FLAGS,
    AREA_DB,
    AREA_COUNTERS,
    AREA_TIMERS,
    isTpkt,
    TpktFrame,
    parseTpkt,
    encodeTpkt,
    CotpHeader,
    parseCotp,
    encodeCotpDt,
    S7Header,
    S7Packet,
    isS7,
    parseS7,
    parseS7OverTpkt,
    S7AnyItem,
    S7AnyItemIter,
} from "cap:iot";

test("1. TPKT encode/parse + COTP DT + S7 Job ReadVar with 12-byte S7-Any address (DB10.DBX12.3)", () => {
    expect_eq(S7_TCP_PORT, 102 as u16);
    expect_eq(TPKT_VERSION, 3 as u8);
    expect_eq(TPKT_HEADER_SIZE, 4 as usize);
    expect_eq(S7_PROTOCOL_ID, 0x32 as u8);
    expect_eq(S7_HEADER_SIZE, 10 as usize);
    expect_eq(S7_ACK_HEADER_SIZE, 12 as usize);

    // Build S7 Job ReadVar requesting 2 items:
    // Item 1: DB10.DBX12.3 (transport=0x01 BIT, len=1, db=10, area=0x84, bit_addr = 12*8 + 3 = 99 = 0x000063)
    // Item 2: MW4 (transport=0x04 WORD, len=2, db=0, area=0x83 FLAGS, bit_addr = 4*8 = 32 = 0x000020)
    let s7_job: [36]u8 = [
        // 10B S7 Job header: proto=0x32, rosctr=1, red=0, ref=0x0102, param_len=26, data_len=0
        0x32, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x1A, 0x00, 0x00,
        // Params: ReadVar (0x04), item_count=2
        0x04, 0x02,
        // Item 1: DB10.DBX12.3
        0x12, 0x0A, 0x10, 0x01, 0x00, 0x01, 0x00, 0x0A, 0x84, 0x00, 0x00, 0x63,
        // Item 2: MW4 (2 words)
        0x12, 0x0A, 0x10, 0x04, 0x00, 0x02, 0x00, 0x00, 0x83, 0x00, 0x00, 0x20,
    ];
    expect_eq(isS7(s7_job[0..]), true);

    // Wrap in COTP DT + TPKT
    let cotp_buf: [64]u8 = std.mem.zeroes([64]u8);
    const cotp_len = encodeCotpDt(s7_job[0..], true, 0, cotp_buf[0..]) ?? unreachable;
    let tpkt_buf: [64]u8 = std.mem.zeroes([64]u8);
    const tpkt_len = encodeTpkt(cotp_buf[0..cotp_len], tpkt_buf[0..]) ?? unreachable;

    expect_eq(isTpkt(tpkt_buf[0..tpkt_len]), true);
    const pkt = parseS7OverTpkt(tpkt_buf[0..tpkt_len]) ?? unreachable;
    expect_eq(pkt.header.rosctr, ROSCTR_JOB);
    expect_eq(pkt.header.pdu_ref, 0x0102 as u16);
    expect_eq(pkt.header.has_error_fields, false);
    expect_eq(pkt.funcCode() ?? 0, FUNC_READ_VAR);
    expect_eq(pkt.data.len, 0 as usize);

    let it = S7AnyItemIter.initFromParams(pkt.params) ?? unreachable;

    const item1 = it.next() ?? unreachable;
    expect_eq(item1.transport_size, 0x01 as u8);
    expect_eq(item1.length, 1 as u16);
    expect_eq(item1.db_number, 10 as u16);
    expect_eq(item1.area, AREA_DB);
    expect_eq(item1.byte_addr, 12 as u32);
    expect_eq(item1.bit_offset, 3 as u8);

    const item2 = it.next() ?? unreachable;
    expect_eq(item2.transport_size, 0x04 as u8);
    expect_eq(item2.length, 2 as u16);
    expect_eq(item2.db_number, 0 as u16);
    expect_eq(item2.area, AREA_FLAGS);
    expect_eq(item2.byte_addr, 4 as u32);
    expect_eq(item2.bit_offset, 0 as u8);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("2. S7 Ack_Data 12-byte header with error_class and error_code + data section", () => {
    // 12B Ack_Data header: proto=0x32, rosctr=3, red=0, ref=0x0005, param_len=2, data_len=5, err_class=0x81, err_code=0x04
    let ack_data: [19]u8 = [
        0x32, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x02, 0x00, 0x05, 0x81, 0x04,
        // Params: ReadVar response, item_count=1
        0x04, 0x01,
        // Data: return_code=0xFF (Success), transport=0x03 (BIT), len=1, value=0x01, fill=0x00
        0xFF, 0x03, 0x00, 0x01, 0x01,
    ];

    const pkt = parseS7(ack_data[0..]) ?? unreachable;
    expect_eq(pkt.header.rosctr, ROSCTR_ACK_DATA);
    expect_eq(pkt.header.pdu_ref, 5 as u16);
    expect_eq(pkt.header.has_error_fields, true);
    expect_eq(pkt.header.error_class, 0x81 as u8);
    expect_eq(pkt.header.error_code, 0x04 as u8);
    expect_eq(pkt.funcCode() ?? 0, FUNC_READ_VAR);
    expect_eq(pkt.params.len, 2 as usize);
    expect_eq(pkt.data.len, 5 as usize);
    expect_eq(pkt.data[0], 0xFF as u8);
});

test("3. Setup Communication (0xF0) Job and COTP CR/CC handshake frames", () => {
    // S7 Setup Communication Job: param_len=8, data_len=0
    // Params: [0xF0, 0x00, max_amq_calling=0x0001, max_amq_called=0x0001, pdu_len=0x01E0 (480)]
    let setup_job: [18]u8 = [
        0x32, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00,
        0xF0, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0xE0,
    ];
    const pkt = parseS7(setup_job[0..]) ?? unreachable;
    expect_eq(pkt.header.rosctr, ROSCTR_JOB);
    expect_eq(pkt.funcCode() ?? 0, FUNC_SETUP_COMM);
    expect_eq(pkt.params.len, 8 as usize);

    // COTP CR (Connection Request) inside TPKT: LI=17 (18B COTP header), PDU type=0xE0
    let cotp_cr: [22]u8 = [
        0x03, 0x00, 0x00, 0x16,
        0x11, 0xE0, 0x00, 0x00, 0x00, 0x01, 0x00,
        0xC0, 0x01, 0x0A,
        0xC1, 0x02, 0x01, 0x00,
        0xC2, 0x02, 0x01, 0x02,
    ];
    const tpkt = parseTpkt(cotp_cr[0..]) ?? unreachable;
    expect_eq(tpkt.length, 22 as u16);
    const cotp = parseCotp(tpkt.payload) ?? unreachable;
    expect_eq(cotp.pdu_type, COTP_CR);
    expect_eq(cotp.li, 17 as u8);
    expect_eq(cotp.payload.len, 0 as usize);

    // parseS7OverTpkt must reject COTP CR (only COTP DT carries S7 payloads)
    expect_eq(parseS7OverTpkt(cotp_cr[0..]) == null, true);
});

test("4. S7 Userdata (rosctr=7) packet parsing and memory area constants", () => {
    expect_eq(AREA_SYS_INFO, 0x03 as u8);
    expect_eq(AREA_INPUTS, 0x81 as u8);
    expect_eq(AREA_OUTPUTS, 0x82 as u8);
    expect_eq(AREA_FLAGS, 0x83 as u8);
    expect_eq(AREA_DB, 0x84 as u8);
    expect_eq(AREA_COUNTERS, 0x1C as u8);
    expect_eq(AREA_TIMERS, 0x1D as u8);

    // Userdata SZL read request: 10B header, param_len=8, data_len=4
    let ud: [22]u8 = [
        0x32, 0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x08, 0x00, 0x04,
        // Params (8B)
        0x00, 0x01, 0x12, 0x04, 0x11, 0x44, 0x01, 0x00,
        // Data (4B)
        0xFF, 0x09, 0x00, 0x00,
    ];
    const pkt = parseS7(ud[0..]) ?? unreachable;
    expect_eq(pkt.header.rosctr, ROSCTR_USERDATA);
    expect_eq(pkt.header.has_error_fields, false);
    expect_eq(pkt.header.pdu_ref, 9 as u16);
    expect_eq(pkt.params.len, 8 as usize);
    expect_eq(pkt.data.len, 4 as usize);
});

test("5. Malformed TPKT version, bad protocol_id, invalid ROSCTR, and truncated payload rejection", () => {
    // Bad TPKT version (2 instead of 3) & length mismatch
    let bad_tpkt: [4]u8 = [0x02, 0x00, 0x00, 0x04];
    expect_eq(isTpkt(bad_tpkt[0..]), false);
    expect_eq(parseTpkt(bad_tpkt[0..]) == null, true);

    let trunc_tpkt: [4]u8 = [0x03, 0x00, 0x00, 0x10];
    expect_eq(parseTpkt(trunc_tpkt[0..]) == null, true);

    // Bad S7 protocol_id (0x33 instead of 0x32)
    let bad_proto: [10]u8 = [0x33, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00];
    expect_eq(isS7(bad_proto[0..]), false);
    expect_eq(parseS7(bad_proto[0..]) == null, true);

    // Invalid ROSCTR (5 is not in {1,2,3,7})
    let bad_rosctr: [10]u8 = [0x32, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00];
    expect_eq(parseS7(bad_rosctr[0..]) == null, true);

    // Ack_Data truncated to 11 bytes (requires 12B header)
    let short_ack: [11]u8 = [0x32, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00];
    expect_eq(parseS7(short_ack[0..]) == null, true);

    // Param/data length exceeds buffer
    let over_len: [10]u8 = [0x32, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00];
    expect_eq(parseS7(over_len[0..]) == null, true);

    // Malformed S7-Any prefix (syntax_id != 0x10) sets hasError()
    let bad_any: [14]u8 = [
        0x04, 0x01,
        0x12, 0x0A, 0x11, 0x02, 0x00, 0x01, 0x00, 0x01, 0x84, 0x00, 0x00, 0x00,
    ];
    let bad_it = S7AnyItemIter.initFromParams(bad_any[0..]) ?? unreachable;
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.hasError(), true);
});
