// Source: cp/cap/test/test_sflow_production_deep.cp (Production deep tests for cap:sflow)

import { test, expect_eq } from "cap:test";
import {
    SFLOW_UDP_PORT,
    SFLOW_V5,
    ADDR_TYPE_IPV4,
    ADDR_TYPE_IPV6,
    SAMPLE_FLOW,
    SAMPLE_COUNTERS,
    SAMPLE_EXPANDED_FLOW,
    SAMPLE_EXPANDED_COUNTERS,
    REC_RAW_PACKET_HEADER,
    REC_EXTENDED_SWITCH,
    CTR_GENERIC_IF,
    SflowHeader,
    SflowSample,
    SflowSampleIter,
    SflowDatagram,
    SflowRecord,
    SflowRecordIter,
    SflowFlowSample,
    SflowRawPacketHeader,
    SflowExtendedSwitch,
    isSflow,
    parseSflow,
    parseFlowSample,
    parseRawPacketHeader,
    parseExtendedSwitch,
} from "cap:sflow";

fn writeBeU32(buf: *[]u8, off: usize, v: u32) {
    buf.*[off] = ((v >> 24) & 0xFF) as u8;
    buf.*[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf.*[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf.*[off + 3] = (v & 0xFF) as u8;
}

test("1. IPv4 Agent sFlow v5 Datagram with nested FlowSample -> RawPacketHeader + ExtendedSwitch", () => {
    expect_eq(SFLOW_UDP_PORT, 6343);
    expect_eq(SFLOW_V5, 5);

    // 布局:
    // Datagram Header (IPv4): 28B @ 0..28
    // Sample 0 (FlowSample, format=1): 8B TLV hdr + 32B flow hdr + Rec0 (8+16+14=38B, pad to 40? XDR raw bytes=16+14=30, let's use 16B captured header -> 8+32=40B) + Rec1 (8+16=24B)
    // Sample 0 body = 32 + 40 + 24 = 96B -> Sample 0 total = 104B @ 28..132
    let buf: [132]u8 = [0; 132];
    let s: []u8 = buf[0..132];

    // Datagram Header (28B)
    writeBeU32(*s, 0, SFLOW_V5);
    writeBeU32(*s, 4, ADDR_TYPE_IPV4);
    s[8] = 10; s[9] = 1; s[10] = 2; s[11] = 3; // agent_ip = 10.1.2.3
    writeBeU32(*s, 12, 0);        // sub_agent_id
    writeBeU32(*s, 16, 1001);     // seq_num
    writeBeU32(*s, 20, 555000);   // uptime_ms
    writeBeU32(*s, 24, 1);        // num_samples = 1

    // Sample 0 TLV header @ 28..36
    writeBeU32(*s, 28, SAMPLE_FLOW); // enterprise=0, format=1
    writeBeU32(*s, 32, 96);          // sample len = 96

    // FlowSample header @ 36..68 (32B)
    writeBeU32(*s, 36, 50);          // seq_num
    writeBeU32(*s, 40, (0 << 24) | 42); // source_id: type=0 (ifIndex), index=42
    writeBeU32(*s, 44, 1024);        // sampling_rate = 1:1024
    writeBeU32(*s, 48, 51200);       // sample_pool
    writeBeU32(*s, 52, 0);           // drops
    writeBeU32(*s, 56, 42);          // input_if
    writeBeU32(*s, 60, 43);          // output_if
    writeBeU32(*s, 64, 2);           // num_records = 2

    // Record 0: RawPacketHeader @ 68..108 (8B hdr + 32B body = 16B meta + 16B captured)
    writeBeU32(*s, 68, REC_RAW_PACKET_HEADER);
    writeBeU32(*s, 72, 32);
    writeBeU32(*s, 76, 1);    // protocol = Ethernet
    writeBeU32(*s, 80, 1518); // frame_len
    writeBeU32(*s, 84, 4);    // stripped (FCS)
    writeBeU32(*s, 88, 16);   // orig_len = 16
    s[92] = 0xAA; s[107] = 0xBB;

    // Record 1: ExtendedSwitch @ 108..132 (8B hdr + 16B body)
    writeBeU32(*s, 108, REC_EXTENDED_SWITCH);
    writeBeU32(*s, 112, 16);
    writeBeU32(*s, 116, 100); // src_vlan
    writeBeU32(*s, 120, 3);   // src_priority
    writeBeU32(*s, 124, 200); // dst_vlan
    writeBeU32(*s, 128, 5);   // dst_priority

    expect_eq(isSflow(buf[0..132]), true);
    const dg = parseSflow(buf[0..132]) ?? unreachable;
    expect_eq(dg.header.isIpv4(), true);
    expect_eq(dg.header.isIpv6(), false);
    expect_eq(dg.header.agent_ip[0], 10);
    expect_eq(dg.header.agent_ip[3], 3);
    expect_eq(dg.header.seq_num, 1001);
    expect_eq(dg.header.uptime_ms, 555000);
    expect_eq(dg.header.num_samples, 1);

    let sit = dg.samples();
    const smp = sit.next() ?? unreachable;
    expect_eq(smp.enterprise, 0);
    expect_eq(smp.format, SAMPLE_FLOW);
    expect_eq(sit.next() == null, true);
    expect_eq(sit.had_error, false);

    const fs = parseFlowSample(smp) ?? unreachable;
    expect_eq(fs.seq_num, 50);
    expect_eq(fs.source_id_type, 0);
    expect_eq(fs.source_id_index, 42);
    expect_eq(fs.sampling_rate, 1024);
    expect_eq(fs.input_if, 42);
    expect_eq(fs.output_if, 43);

    let rit = fs.records();
    const r0 = rit.next() ?? unreachable;
    const raw = parseRawPacketHeader(r0) ?? unreachable;
    expect_eq(raw.protocol, 1);
    expect_eq(raw.frame_len, 1518);
    expect_eq(raw.stripped, 4);
    expect_eq(raw.orig_len, 16);
    expect_eq(raw.header_bytes.len, 16);
    expect_eq(raw.header_bytes[0], 0xAA);
    expect_eq(raw.header_bytes[15], 0xBB);

    const r1 = rit.next() ?? unreachable;
    const sw = parseExtendedSwitch(r1) ?? unreachable;
    expect_eq(sw.src_vlan, 100);
    expect_eq(sw.src_priority, 3);
    expect_eq(sw.dst_vlan, 200);
    expect_eq(sw.dst_priority, 5);

    expect_eq(rit.next() == null, true);
    expect_eq(rit.had_error, false);
});

test("2. IPv6 Agent header (40B) + Expanded Flow Sample (format=3)", () => {
    // 40B IPv6 Datagram Header + 8B Sample TLV + 44B Expanded Flow Sample = 92 bytes
    let buf: [92]u8 = [0; 92];
    let s: []u8 = buf[0..92];

    writeBeU32(*s, 0, SFLOW_V5);
    writeBeU32(*s, 4, ADDR_TYPE_IPV6);
    s[8] = 0x20; s[9] = 0x01; s[10] = 0x0d; s[11] = 0xb8; s[23] = 0x09; // 2001:db8::9
    writeBeU32(*s, 24, 7);    // sub_agent_id
    writeBeU32(*s, 28, 202);  // seq_num
    writeBeU32(*s, 32, 9999); // uptime_ms
    writeBeU32(*s, 36, 1);    // num_samples

    // Expanded Flow Sample (format=3, len=44) @ 40..92
    writeBeU32(*s, 40, SAMPLE_EXPANDED_FLOW);
    writeBeU32(*s, 44, 44);
    writeBeU32(*s, 48, 88);     // seq_num
    writeBeU32(*s, 52, 2);      // source_id_type
    writeBeU32(*s, 56, 70000);  // source_id_index (> 16-bit)
    writeBeU32(*s, 60, 512);    // sampling_rate
    writeBeU32(*s, 64, 100000); // sample_pool
    writeBeU32(*s, 68, 3);      // drops
    writeBeU32(*s, 72, 0);      // input_if_format
    writeBeU32(*s, 76, 101);    // input_if
    writeBeU32(*s, 80, 0);      // output_if_format
    writeBeU32(*s, 84, 102);    // output_if
    writeBeU32(*s, 88, 0);      // num_records = 0

    const dg = parseSflow(buf[0..92]) ?? unreachable;
    expect_eq(dg.header.isIpv6(), true);
    expect_eq(dg.header.agent_ip[0], 0x20);
    expect_eq(dg.header.agent_ip[15], 0x09);
    expect_eq(dg.header.sub_agent_id, 7);

    let sit = dg.samples();
    const smp = sit.next() ?? unreachable;
    expect_eq(smp.format, SAMPLE_EXPANDED_FLOW);
    const fs = parseFlowSample(smp) ?? unreachable;
    expect_eq(fs.seq_num, 88);
    expect_eq(fs.source_id_type, 2);
    expect_eq(fs.source_id_index, 70000);
    expect_eq(fs.drops, 3);
    expect_eq(fs.input_if, 101);
    expect_eq(fs.output_if, 102);
});

test("3. Counter Sample (format=2) & vendor enterprise sample identification", () => {
    // 28B v4 header + Counter Sample (8+12=20B) + Vendor Sample (enterprise=4300, format=5, 8+4=12B) = 60B
    let buf: [60]u8 = [0; 60];
    let s: []u8 = buf[0..60];
    writeBeU32(*s, 0, SFLOW_V5);
    writeBeU32(*s, 4, ADDR_TYPE_IPV4);
    writeBeU32(*s, 24, 2); // num_samples = 2

    // Sample 0: Counter Sample
    writeBeU32(*s, 28, SAMPLE_COUNTERS);
    writeBeU32(*s, 32, 12);

    // Sample 1: Enterprise 4300, format 5
    writeBeU32(*s, 48, (4300 << 12) | 5);
    writeBeU32(*s, 52, 4);

    const dg = parseSflow(buf[0..60]) ?? unreachable;
    let sit = dg.samples();

    const s0 = sit.next() ?? unreachable;
    expect_eq(s0.enterprise, 0);
    expect_eq(s0.format, SAMPLE_COUNTERS);
    expect_eq(parseFlowSample(s0) == null, true); // Counter sample is not FlowSample

    const s1 = sit.next() ?? unreachable;
    expect_eq(s1.enterprise, 4300);
    expect_eq(s1.format, 5);
    expect_eq(s1.data.len, 4);
    expect_eq(parseFlowSample(s1) == null, true);
    expect_eq(sit.had_error, false);
});

test("4. Record type mismatch guards for parseRawPacketHeader & parseExtendedSwitch", () => {
    const wrong_rec = SflowRecord {
        enterprise: 0,
        format: CTR_GENERIC_IF,
        data: [0; 20][0..20],
    };
    expect_eq(parseRawPacketHeader(wrong_rec) == null, true);
    expect_eq(parseExtendedSwitch(wrong_rec) == null, true);

    // RawPacketHeader orig_len exceeds data len
    let bad_raw: [20]u8 = [0; 20];
    let br: []u8 = bad_raw[0..20];
    writeBeU32(*br, 12, 64); // orig_len = 64, but only 4 bytes remain
    const rec_oob = SflowRecord {
        enterprise: 0,
        format: REC_RAW_PACKET_HEADER,
        data: bad_raw[0..20],
    };
    expect_eq(parseRawPacketHeader(rec_oob) == null, true);
});

test("5. Malformed sFlow defense: bad version, bad addr_type, truncated IPv6 hdr, OOB sample/record", () => {
    // 5.1 短于 28B
    const short20: [20]u8 = [0; 20];
    expect_eq(isSflow(short20[0..20]), false);
    expect_eq(parseSflow(short20[0..20]) == null, true);

    // 5.2 非 v5 (v4)
    let v4: [28]u8 = [0; 28];
    let v4s: []u8 = v4[0..28];
    writeBeU32(*v4s, 0, 4);
    writeBeU32(*v4s, 4, ADDR_TYPE_IPV4);
    expect_eq(parseSflow(v4[0..28]) == null, true);

    // 5.3 非法 addr_type = 3
    writeBeU32(*v4s, 0, SFLOW_V5);
    writeBeU32(*v4s, 4, 3);
    expect_eq(parseSflow(v4[0..28]) == null, true);

    // 5.4 IPv6 addr_type 但总长不足 40B
    writeBeU32(*v4s, 4, ADDR_TYPE_IPV6);
    expect_eq(parseSflow(v4[0..28]) == null, true);

    // 5.5 Sample 声明长度越界 -> SampleIter 置 had_error = true
    let bad_smp: [12]u8 = [0; 12];
    let bs: []u8 = bad_smp[0..12];
    writeBeU32(*bs, 0, SAMPLE_FLOW);
    writeBeU32(*bs, 4, 100); // claims 100B, only 4B left
    let sit = SflowSampleIter.init(bad_smp[0..12], 1);
    expect_eq(sit.next() == null, true);
    expect_eq(sit.had_error, true);
});
