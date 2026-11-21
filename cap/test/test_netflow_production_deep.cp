// Source: cp/cap/test/test_netflow_production_deep.cp
// Deep Production Test Suite for cap:netflow
// Covers:
//   1. NetFlow v5 header (sampling_mode + sampling_interval) & 2 x 48B Flow Record iteration
//   2. NetFlow v9 Template FlowSet + Nf9TemplateIter + Nf9FieldSpecIter + Data FlowSet iteration
//   3. NetFlow v9 Options Template FlowSet (id == 1) recognition
//   4. peekNetflowVersion() & isNetflow() version detection
//   5. NetFlow v5 strict guards: bad version, count == 0, count == 31 (> 30), truncated records
//   6. NetFlow v9 FlowSet length < 4, truncated FlowSet, and malformed Template Record guards

import { test, expect_eq } from "cap:test";
import {
    NETFLOW_UDP_PORT,
    NETFLOW_V5,
    NETFLOW_V9,
    NF5_HEADER_SIZE,
    NF5_RECORD_SIZE,
    NF9_HEADER_SIZE,
    NF9_FLOWSET_HEADER_SIZE,
    NF9_FLOWSET_TEMPLATE,
    NF9_FLOWSET_OPTIONS_TEMPLATE,
    NF9_FLOWSET_DATA_MIN,
    Nf5Header,
    Nf5Record,
    Nf5RecordIter,
    Nf5Packet,
    Nf9Header,
    Nf9FieldSpec,
    Nf9FieldSpecIter,
    Nf9TemplateRecord,
    Nf9TemplateIter,
    Nf9FlowSet,
    Nf9FlowSetIter,
    Nf9Packet,
    peekNetflowVersion,
    parseNetflowV5,
    parseNetflowV9,
    isNetflow,
} from "cap:netflow";

fn ip4Eq(a: [4]u8, b: [4]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

test("netflow: NetFlow v5 header (sampling) + 2 x 48B Flow Record iteration") {
    expect_eq(NETFLOW_UDP_PORT, 2055);
    expect_eq(NF5_HEADER_SIZE, 24);
    expect_eq(NF5_RECORD_SIZE, 48);

    // 24B header + 2 * 48B records = 120B
    let pkt: [120]u8 = [0; 120];
    // version=5, count=2
    pkt[1] = 5;
    pkt[3] = 2;
    // sys_uptime_ms = 0x00010000
    pkt[5] = 0x01;
    // unix_secs = 1700000000 (0x6553F100)
    pkt[8] = 0x65; pkt[9] = 0x53; pkt[10] = 0xF1; pkt[11] = 0x00;
    // flow_sequence = 42
    pkt[19] = 42;
    // engine_type=1, engine_id=2
    pkt[20] = 1; pkt[21] = 2;
    // sampling_mode=1 (bit 14 -> 0x4000) | sampling_interval=1000 (0x03E8) = 0x43E8
    pkt[22] = 0x43; pkt[23] = 0xE8;

    // Record 0 at offset 24: 10.0.0.1:443 -> 192.168.1.5:51234, TCP(6), 10 pkts, 1500 octets, AS 65001->65002
    const r0: usize = 24;
    pkt[r0 + 0] = 10; pkt[r0 + 1] = 0; pkt[r0 + 2] = 0; pkt[r0 + 3] = 1;
    pkt[r0 + 4] = 192; pkt[r0 + 5] = 168; pkt[r0 + 6] = 1; pkt[r0 + 7] = 5;
    pkt[r0 + 19] = 10;     // packets
    pkt[r0 + 22] = 0x05; pkt[r0 + 23] = 0xDC; // 1500 octets
    pkt[r0 + 32] = 0x01; pkt[r0 + 33] = 0xBB; // src_port = 443
    pkt[r0 + 34] = 0xC8; pkt[r0 + 35] = 0x22; // dst_port = 51234
    pkt[r0 + 37] = 0x18;   // PSH+ACK
    pkt[r0 + 38] = 6;      // TCP
    pkt[r0 + 40] = 0xFD; pkt[r0 + 41] = 0xE9; // src_as = 65001
    pkt[r0 + 42] = 0xFD; pkt[r0 + 43] = 0xEA; // dst_as = 65002
    pkt[r0 + 44] = 24; pkt[r0 + 45] = 24;

    // Record 1 at offset 72: 172.16.0.1:53 -> 8.8.8.8:53, UDP(17), 1 pkt, 64 octets
    const r1: usize = 72;
    pkt[r1 + 0] = 172; pkt[r1 + 1] = 16; pkt[r1 + 2] = 0; pkt[r1 + 3] = 1;
    pkt[r1 + 4] = 8; pkt[r1 + 5] = 8; pkt[r1 + 6] = 8; pkt[r1 + 7] = 8;
    pkt[r1 + 19] = 1;
    pkt[r1 + 23] = 64;
    pkt[r1 + 33] = 53;
    pkt[r1 + 35] = 53;
    pkt[r1 + 38] = 17; // UDP

    expect_eq(peekNetflowVersion(pkt[0..120]) == NETFLOW_V5, true);
    expect_eq(isNetflow(pkt[0..120]), true);

    if (nf5 := parseNetflowV5(pkt[0..120])) {
        expect_eq(nf5.header.version, 5);
        expect_eq(nf5.header.count, 2);
        expect_eq(nf5.header.flow_sequence, 42);
        expect_eq(nf5.header.engine_type, 1);
        expect_eq(nf5.header.engine_id, 2);
        expect_eq(nf5.header.sampling_mode, 1);
        expect_eq(nf5.header.sampling_interval, 1000);

        let it = nf5.records();
        let idx: u32 = 0;
        while (rec := it.next()) {
            idx += 1;
            if (idx == 1) {
                expect_eq(ip4Eq(rec.src_ip, [10, 0, 0, 1]), true);
                expect_eq(ip4Eq(rec.dst_ip, [192, 168, 1, 5]), true);
                expect_eq(rec.packets, 10);
                expect_eq(rec.octets, 1500);
                expect_eq(rec.src_port, 443);
                expect_eq(rec.dst_port, 51234);
                expect_eq(rec.tcp_flags, 0x18);
                expect_eq(rec.protocol, 6);
                expect_eq(rec.src_as, 65001);
                expect_eq(rec.dst_as, 65002);
                expect_eq(rec.src_mask, 24);
            } else if (idx == 2) {
                expect_eq(ip4Eq(rec.src_ip, [172, 16, 0, 1]), true);
                expect_eq(ip4Eq(rec.dst_ip, [8, 8, 8, 8]), true);
                expect_eq(rec.packets, 1);
                expect_eq(rec.octets, 64);
                expect_eq(rec.src_port, 53);
                expect_eq(rec.protocol, 17);
            }
        }
        expect_eq(idx, 2);
    } else {
        expect_eq(true, false);
    }
}

test("netflow: NetFlow v9 Template FlowSet + FieldSpec + Data FlowSet iteration") {
    expect_eq(NF9_HEADER_SIZE, 20);
    expect_eq(NF9_FLOWSET_HEADER_SIZE, 4);

    // 20B Header + 16B Template FlowSet (id=0, len=16, tpl=256, field_count=2: [8,4], [12,4]) +
    // 12B Data FlowSet (id=256, len=12, 8B payload) = 48B
    let pkt: [48]u8 = [
        // 20B Header: ver=9, count=2, uptime=1000, unix=1700000000, seq=7, source_id=0x11223344
        0x00, 0x09, 0x00, 0x02,
        0x00, 0x00, 0x03, 0xE8,
        0x65, 0x53, 0xF1, 0x00,
        0x00, 0x00, 0x00, 0x07,
        0x11, 0x22, 0x33, 0x44,
        // FlowSet 1: Template (id=0, len=16)
        0x00, 0x00, 0x00, 16,
        0x01, 0x00, 0x00, 0x02, // template_id = 256, field_count = 2
        0x00, 0x08, 0x00, 0x04, // IPV4_SRC_ADDR (8), len=4
        0x00, 0x0C, 0x00, 0x04, // IPV4_DST_ADDR (12), len=4
        // FlowSet 2: Data (id=256, len=12)
        0x01, 0x00, 0x00, 12,
        10, 0, 0, 1, 10, 0, 0, 2,
    ];

    expect_eq(peekNetflowVersion(pkt[0..48]) == NETFLOW_V9, true);
    expect_eq(isNetflow(pkt[0..48]), true);

    if (nf9 := parseNetflowV9(pkt[0..48])) {
        expect_eq(nf9.header.version, 9);
        expect_eq(nf9.header.count, 2);
        expect_eq(nf9.header.sequence_number, 7);
        expect_eq(nf9.header.source_id, 0x11223344);

        let fs_it = nf9.flowsets();
        let fs_idx: u32 = 0;
        while (fs := fs_it.next()) {
            fs_idx += 1;
            if (fs_idx == 1) {
                expect_eq(fs.isTemplate(), true);
                expect_eq(fs.isOptionsTemplate(), false);
                expect_eq(fs.isData(), false);

                let t_it = fs.templates();
                if (tpl := t_it.next()) {
                    expect_eq(tpl.template_id, 256);
                    expect_eq(tpl.field_count, 2);
                    let f_it = tpl.fields();
                    if (f1 := f_it.next()) {
                        expect_eq(f1.field_type, 8);
                        expect_eq(f1.field_len, 4);
                    } else {
                        expect_eq(true, false);
                    }
                    if (f2 := f_it.next()) {
                        expect_eq(f2.field_type, 12);
                        expect_eq(f2.field_len, 4);
                    } else {
                        expect_eq(true, false);
                    }
                    expect_eq(f_it.next() == null, true);
                } else {
                    expect_eq(true, false);
                }
                expect_eq(t_it.next() == null, true);
                expect_eq(t_it.hasError(), false);
            } else if (fs_idx == 2) {
                expect_eq(fs.isData(), true);
                expect_eq(fs.flowset_id, 256);
                expect_eq(fs.body.len, 8);
                expect_eq(fs.body[0], 10);
                expect_eq(fs.body[7], 2);
            }
        }
        expect_eq(fs_idx, 2);
        expect_eq(fs_it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("netflow: NetFlow v9 Options Template FlowSet recognition") {
    let pkt: [28]u8 = [0; 28];
    pkt[1] = 9;
    pkt[3] = 1;
    // Options Template FlowSet: id=1, len=8
    pkt[21] = 1;
    pkt[23] = 8;

    if (nf9 := parseNetflowV9(pkt[0..28])) {
        let fs_it = nf9.flowsets();
        if (fs := fs_it.next()) {
            expect_eq(fs.isOptionsTemplate(), true);
            expect_eq(fs.isTemplate(), false);
            expect_eq(fs.isData(), false);
            expect_eq(fs.body.len, 4);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("netflow: NetFlow v5 strict guards (bad version, count=0 / >30, truncated)") {
    expect_eq(peekNetflowVersion("\x00") == null, true);
    expect_eq(isNetflow("\x00\x05"), false);

    // version = 10 (IPFIX, not v5 or v9)
    let v10: [24]u8 = [0; 24];
    v10[1] = 10; v10[3] = 1;
    expect_eq(isNetflow(v10[0..24]), false);

    // v5 count = 0 -> rejected
    let c0: [72]u8 = [0; 72];
    c0[1] = 5; c0[3] = 0;
    expect_eq(parseNetflowV5(c0[0..72]) == null, true);

    // v5 count = 31 (> 30) -> rejected
    let c31: [72]u8 = [0; 72];
    c31[1] = 5; c31[3] = 31;
    expect_eq(parseNetflowV5(c31[0..72]) == null, true);

    // v5 count = 2 needs 120B, given 72B -> rejected
    let trunc: [72]u8 = [0; 72];
    trunc[1] = 5; trunc[3] = 2;
    expect_eq(parseNetflowV5(trunc[0..72]) == null, true);
}

test("netflow: NetFlow v9 FlowSet length<4, truncated FlowSet & malformed Template guards") {
    // FlowSet length = 2 (< 4) -> hasError()
    let fs_it1 = Nf9FlowSetIter.init("\x00\x00\x00\x02");
    expect_eq(fs_it1.next() == null, true);
    expect_eq(fs_it1.hasError(), true);

    // FlowSet claims length = 20, only 8 bytes available -> hasError()
    let fs_it2 = Nf9FlowSetIter.init("\x01\x00\x00\x14\x01\x02\x03\x04");
    expect_eq(fs_it2.next() == null, true);
    expect_eq(fs_it2.hasError(), true);

    // Template Record with template_id = 100 (< 256) -> hasError()
    let t_it1 = Nf9TemplateIter.init("\x00\x64\x00\x01\x00\x08\x00\x04");
    expect_eq(t_it1.next() == null, true);
    expect_eq(t_it1.hasError(), true);

    // Template Record claims 5 fields (20B), only 4B available -> hasError()
    let t_it2 = Nf9TemplateIter.init("\x01\x00\x00\x05\x00\x08\x00\x04");
    expect_eq(t_it2.next() == null, true);
    expect_eq(t_it2.hasError(), true);
}
