// Source: cp/cap/test/test_ptp_production_deep.cp (Production deep tests for cap:ptp)

import { test, expect_eq } from "cap:test";
import {
    PTP_ETHERTYPE,
    PTP_UDP_EVENT_PORT,
    PTP_UDP_GENERAL_PORT,
    PTP_V2,
    PTP_HEADER_SIZE,
    PTP_TIMESTAMP_SIZE,
    MSG_SYNC,
    MSG_DELAY_REQ,
    MSG_PDELAY_REQ,
    MSG_PDELAY_RESP,
    MSG_FOLLOW_UP,
    MSG_DELAY_RESP,
    MSG_PDELAY_RESP_FOLLOW_UP,
    MSG_ANNOUNCE,
    MSG_SIGNALING,
    MSG_MANAGEMENT,
    FLAG_ALTERNATE_MASTER,
    FLAG_TWO_STEP,
    FLAG_UNICAST,
    FLAG_LEAP61,
    FLAG_LEAP59,
    FLAG_UTC_OFFSET_VALID,
    FLAG_PTP_TIMESCALE,
    FLAG_TIME_TRACEABLE,
    FLAG_FREQ_TRACEABLE,
    PtpTimestamp,
    PtpPortIdentity,
    PtpHeader,
    PtpPacket,
    PtpAnnounceBody,
    PtpDelayRespBody,
    isPtp,
    parsePtp,
    parsePtpTimestamp,
    parsePtpAnnounce,
    parsePtpDelayResp,
    encodePtpHeader,
} from "cap:time";

fn writeTs(buf: *[]u8, off: usize, secs: u64, nanos: u32) {
    buf.*[off] = ((secs >> 40) & 0xFF) as u8;
    buf.*[off + 1] = ((secs >> 32) & 0xFF) as u8;
    buf.*[off + 2] = ((secs >> 24) & 0xFF) as u8;
    buf.*[off + 3] = ((secs >> 16) & 0xFF) as u8;
    buf.*[off + 4] = ((secs >> 8) & 0xFF) as u8;
    buf.*[off + 5] = (secs & 0xFF) as u8;
    buf.*[off + 6] = ((nanos >> 24) & 0xFF) as u8;
    buf.*[off + 7] = ((nanos >> 16) & 0xFF) as u8;
    buf.*[off + 8] = ((nanos >> 8) & 0xFF) as u8;
    buf.*[off + 9] = (nanos & 0xFF) as u8;
}

test("1. Sync & Follow_Up encodePtpHeader roundtrip + 48-bit seconds timestamp & signed correction_field", () => {
    expect_eq(PTP_ETHERTYPE, 0x88F7);
    expect_eq(PTP_UDP_EVENT_PORT, 319);
    expect_eq(PTP_UDP_GENERAL_PORT, 320);
    expect_eq(PTP_V2, 2);
    expect_eq(PTP_HEADER_SIZE, 34);
    expect_eq(PTP_TIMESTAMP_SIZE, 10);

    // 构造 44 字节 Sync 报文 (34B header + 10B originTimestamp)
    let buf: [44]u8 = [0; 44];
    let out: []u8 = buf[0..44];
    const hdr = PtpHeader {
        major_sdo_id: 1,
        msg_type: MSG_SYNC,
        minor_version: 1, // PTPv2.1
        version: PTP_V2,
        message_length: 44,
        domain_number: 24,
        flags: FLAG_TWO_STEP | FLAG_UNICAST | FLAG_PTP_TIMESCALE,
        correction_field: -((1250 as i64) << 16), // -1250 ns 负修正值
        source_port_id: PtpPortIdentity {
            clock_id: [0x00, 0x1B, 0x19, 0xFF, 0xFE, 0x00, 0x00, 0x01],
            port_number: 1,
        },
        sequence_id: 0x0401,
        control_field: 0,
        log_message_interval: -3, // 1/8 sec (8 Hz)
    };

    const hn = encodePtpHeader(hdr, *out) ?? unreachable;
    expect_eq(hn, 34);

    // 写入 48-bit 秒时间戳（超过 32-bit 上限：0x0001_0000_0005 = 4294967301 s, 987654321 ns）
    const big_sec: u64 = 0x000100000005;
    writeTs(*out, 34, big_sec, 987654321);

    expect_eq(isPtp(out[0..44]), true);
    const pkt = parsePtp(out[0..44]) ?? unreachable;
    expect_eq(pkt.header.major_sdo_id, 1);
    expect_eq(pkt.header.msg_type, MSG_SYNC);
    expect_eq(pkt.header.minor_version, 1);
    expect_eq(pkt.header.version, PTP_V2);
    expect_eq(pkt.header.domain_number, 24);
    expect_eq(pkt.header.isEvent(), true);
    expect_eq(pkt.header.isTwoStep(), true);
    expect_eq(pkt.header.isUnicast(), true);
    expect_eq(pkt.header.correctionNs(), -1250);
    expect_eq(pkt.header.source_port_id.clock_id[1], 0x1B);
    expect_eq(pkt.header.source_port_id.port_number, 1);
    expect_eq(pkt.header.sequence_id, 0x0401);
    expect_eq(pkt.header.log_message_interval, -3);

    const ts = parsePtpTimestamp(pkt.body) ?? unreachable;
    expect_eq(ts.seconds, big_sec);
    expect_eq(ts.nanoseconds, 987654321);

    const total_ns: u128 = ts.toNanoseconds();
    const back = PtpTimestamp.fromNanoseconds(total_ns);
    expect_eq(back.seconds, big_sec);
    expect_eq(back.nanoseconds, 987654321);
});

test("2. Announce (0xB) full Grandmaster fields & UTC offset parsing", () => {
    // 34B header + 30B Announce body = 64 bytes
    let buf: [64]u8 = [0; 64];
    let out: []u8 = buf[0..64];
    const hdr = PtpHeader {
        major_sdo_id: 0,
        msg_type: MSG_ANNOUNCE,
        minor_version: 0,
        version: PTP_V2,
        message_length: 64,
        domain_number: 0,
        flags: FLAG_UTC_OFFSET_VALID | FLAG_PTP_TIMESCALE | FLAG_TIME_TRACEABLE | FLAG_FREQ_TRACEABLE,
        correction_field: (500 as i64) << 16,
        source_port_id: PtpPortIdentity {
            clock_id: [0xAA, 0xBB, 0xCC, 0xFF, 0xFE, 0x11, 0x22, 0x33],
            port_number: 2,
        },
        sequence_id: 99,
        control_field: 5,
        log_message_interval: 1,
    };
    * = encodePtpHeader(hdr, *out);

    // Announce body @ 34..64
    writeTs(*out, 34, 1700000000, 123456);
    // current_utc_offset = 37 (TAI-UTC)
    out[44] = 0x00; out[45] = 37;
    out[46] = 0;    // reserved
    out[47] = 128;  // gm_priority1
    out[48] = 6;    // gm_clock_class (GPS locked)
    out[49] = 0x20; // gm_clock_accuracy (25ns)
    out[50] = 0x4E; out[51] = 0x5D; // gm_offset_scaled_log_variance
    out[52] = 128;  // gm_priority2
    // gm_identity @ 53..61
    out[53] = 0x01; out[54] = 0x02; out[55] = 0x03; out[56] = 0xFF;
    out[57] = 0xFE; out[58] = 0x04; out[59] = 0x05; out[60] = 0x06;
    // steps_removed = 2 @ 61..63
    out[61] = 0x00; out[62] = 0x02;
    // time_source = 0x20 (GPS) @ 63
    out[63] = 0x20;

    const pkt = parsePtp(out[0..64]) ?? unreachable;
    expect_eq(pkt.header.isEvent(), false);
    expect_eq(pkt.header.correctionNs(), 500);

    const ann = parsePtpAnnounce(pkt.body) ?? unreachable;
    expect_eq(ann.origin_ts.seconds, 1700000000);
    expect_eq(ann.origin_ts.nanoseconds, 123456);
    expect_eq(ann.current_utc_offset, 37);
    expect_eq(ann.gm_priority1, 128);
    expect_eq(ann.gm_clock_class, 6);
    expect_eq(ann.gm_clock_accuracy, 0x20);
    expect_eq(ann.gm_offset_scaled_log_variance, 0x4E5D);
    expect_eq(ann.gm_priority2, 128);
    expect_eq(ann.gm_identity[0], 0x01);
    expect_eq(ann.gm_identity[7], 0x06);
    expect_eq(ann.steps_removed, 2);
    expect_eq(ann.time_source, 0x20);
});

test("3. Delay_Resp (0x9) body parsing with receive_ts & requesting_port_id", () => {
    // 34B header + 20B Delay_Resp body = 54 bytes
    let buf: [54]u8 = [0; 54];
    let out: []u8 = buf[0..54];
    const hdr = PtpHeader {
        major_sdo_id: 0,
        msg_type: MSG_DELAY_RESP,
        minor_version: 0,
        version: PTP_V2,
        message_length: 54,
        domain_number: 0,
        flags: 0,
        correction_field: 0,
        source_port_id: PtpPortIdentity { clock_id: [0; 8], port_number: 1 },
        sequence_id: 7,
        control_field: 3,
        log_message_interval: 0,
    };
    * = encodePtpHeader(hdr, *out);

    writeTs(*out, 34, 1700000005, 500000000);
    // requesting_port_id @ 44..54
    out[44] = 0xDE; out[45] = 0xAD; out[51] = 0xEF;
    out[52] = 0x00; out[53] = 0x05; // port 5

    const pkt = parsePtp(out[0..54]) ?? unreachable;
    expect_eq(pkt.header.msg_type, MSG_DELAY_RESP);
    expect_eq(pkt.header.isEvent(), false);

    const dr = parsePtpDelayResp(pkt.body) ?? unreachable;
    expect_eq(dr.receive_ts.seconds, 1700000005);
    expect_eq(dr.receive_ts.nanoseconds, 500000000);
    expect_eq(dr.requesting_port_id.clock_id[0], 0xDE);
    expect_eq(dr.requesting_port_id.clock_id[7], 0xEF);
    expect_eq(dr.requesting_port_id.port_number, 5);
});

test("4. Follow_Up (0x8) positive fractional correction_field truncation", () => {
    let buf: [44]u8 = [0; 44];
    let out: []u8 = buf[0..44];
    const hdr = PtpHeader {
        major_sdo_id: 0,
        msg_type: MSG_FOLLOW_UP,
        minor_version: 0,
        version: PTP_V2,
        message_length: 44,
        domain_number: 0,
        flags: 0,
        // 42.5 ns = (42 << 16) + 0x8000
        correction_field: ((42 as i64) << 16) + 0x8000,
        source_port_id: PtpPortIdentity { clock_id: [0; 8], port_number: 1 },
        sequence_id: 1,
        control_field: 2,
        log_message_interval: 0,
    };
    * = encodePtpHeader(hdr, *out);
    const pkt = parsePtp(out[0..44]) ?? unreachable;
    expect_eq(pkt.header.correctionNs(), 42);
});

test("5. Malformed PTP defense: bad version, nanoseconds >= 1e9, truncated header/body, small output buffer", () => {
    // 5.1 短于 34B
    const short33: [33]u8 = [0; 33];
    expect_eq(isPtp(short33[0..33]), false);
    expect_eq(parsePtp(short33[0..33]) == null, true);

    // 5.2 非法版本 (v1)
    let bad_ver: [34]u8 = [0; 34];
    bad_ver[1] = 1; // version = 1
    bad_ver[3] = 34;
    expect_eq(isPtp(bad_ver[0..34]), false);
    expect_eq(parsePtp(bad_ver[0..34]) == null, true);

    // 5.3 message_length < 34 或 > data.len
    let bad_len: [34]u8 = [0; 34];
    bad_len[1] = PTP_V2;
    bad_len[3] = 30; // < 34
    expect_eq(parsePtp(bad_len[0..34]) == null, true);
    bad_len[3] = 44; // > 34
    expect_eq(parsePtp(bad_len[0..34]) == null, true);

    // 5.4 时间戳 nanoseconds >= 1_000_000_000 (0x3B9ACA00) 必须拒绝
    let bad_ts: [10]u8 = [0; 10];
    let ts_s: []u8 = bad_ts[0..10];
    writeTs(*ts_s, 0, 100, 1000000000);
    expect_eq(parsePtpTimestamp(ts_s[0..10]) == null, true);

    // 5.5 截断的 Announce (29B) & Delay_Resp (19B)
    const short_ann: [29]u8 = [0; 29];
    expect_eq(parsePtpAnnounce(short_ann[0..29]) == null, true);
    const short_dr: [19]u8 = [0; 19];
    expect_eq(parsePtpDelayResp(short_dr[0..19]) == null, true);

    // 5.6 encodePtpHeader 缓冲不足 (< 34B)
    let small: [33]u8 = [0; 33];
    let ss: []u8 = small[0..33];
    const hdr = PtpHeader {
        major_sdo_id: 0,
        msg_type: MSG_SYNC,
        minor_version: 0,
        version: PTP_V2,
        message_length: 34,
        domain_number: 0,
        flags: 0,
        correction_field: 0,
        source_port_id: PtpPortIdentity { clock_id: [0; 8], port_number: 1 },
        sequence_id: 0,
        control_field: 0,
        log_message_interval: 0,
    };
    expect_eq(encodePtpHeader(hdr, *ss) == null, true);
});
