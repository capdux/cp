// Source: cp/cap/test/test_snmp_production_deep.cp (Production deep tests for cap:snmp)

import { test, expect_eq } from "cap:test";
import {
    SNMP_PORT,
    SNMP_TRAP_PORT,
    SNMP_V1,
    SNMP_V2C,
    SnmpPduType,
    SnmpValueTag,
    SnmpVarBind,
    SnmpVarBindIter,
    SnmpMessage,
    parseSnmpMessage,
    formatSnmpOid,
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

/// 辅助：在栈缓冲区内按 BER 确定长度写入 TLV（支持 len < 65536）
fn appendTlv(buf: *[]u8, pos: *usize, tag: u8, val: []const u8) {
    buf.*[pos.*] = tag;
    pos.* += 1;
    if (val.len < 128) {
        buf.*[pos.*] = val.len as u8;
        pos.* += 1;
    } else if (val.len < 256) {
        buf.*[pos.*] = 0x81;
        buf.*[pos.* + 1] = val.len as u8;
        pos.* += 2;
    } else {
        buf.*[pos.*] = 0x82;
        buf.*[pos.* + 1] = ((val.len >> 8) & 0xFF) as u8;
        buf.*[pos.* + 2] = (val.len & 0xFF) as u8;
        pos.* += 3;
    }
    let i: usize = 0;
    while (i < val.len) {
        buf.*[pos.*] = val[i];
        pos.* += 1;
        i += 1;
    }
}

test("1. SNMP v2c GetRequest & multi-type GetResponse (OctetString, TimeTicks, Counter64, IpAddress, endOfMibView, negative Integer)", () => {
    expect_eq(SNMP_PORT, 161);
    expect_eq(SNMP_TRAP_PORT, 162);
    expect_eq(SNMP_V1, 0);
    expect_eq(SNMP_V2C, 1);

    // 1A) 标准 SNMP v2c GetRequest: community="public", req_id=1001, VarBind=[sysDescr.0 -> NULL]
    const get_req_bytes: [41]u8 = [
        0x30, 0x27, // SEQUENCE (39)
        0x02, 0x01, 0x01, // version = 1 (v2c)
        0x04, 0x06, 112, 117, 98, 108, 105, 99, // "public"
        0xA0, 0x1A, // GetRequest-PDU (26)
        0x02, 0x02, 0x03, 0xE9, // request-id = 1001
        0x02, 0x01, 0x00, // error-status = 0
        0x02, 0x01, 0x00, // error-index = 0
        0x30, 0x0E, // VarBindList (14)
        0x30, 0x0C, // VarBind (12)
        0x06, 0x08, 0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00, // 1.3.6.1.2.1.1.1.0
        0x05, 0x00, // NULL
    ];

    const req = parseSnmpMessage(get_req_bytes[0..41]) ?? unreachable;
    expect_eq(req.version, SNMP_V2C);
    expect_eq(sliceEq(req.community, "public"), true);
    expect_eq(req.pdu_type == .get_request, true);
    expect_eq(req.request_id, 1001);
    expect_eq(req.error_status, 0);
    expect_eq(req.error_index, 0);

    let req_it = req.varbinds();
    const req_vb = req_it.next() ?? unreachable;
    expect_eq(req_vb.isNull(), true);
    expect_eq(req_vb.isException(), false);
    expect_eq(req_it.next() == null, true);
    expect_eq(req_it.had_error, false);

    // 1B) 构造多 VarBind SNMP v2c GetResponse
    let vbl_buf: [256]u8 = [0; 256];
    let vbl: []u8 = vbl_buf[0..256];
    let vbl_pos: usize = 0;

    // Helper buffer for individual VarBinds
    let tmp_buf: [128]u8 = [0; 128];
    let tmp: []u8 = tmp_buf[0..128];

    // VB1: sysDescr.0 (1.3.6.1.2.1.1.1.0) -> OCTET STRING "CP-Router"
    const oid_sys_descr: [8]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00];
    let tpos: usize = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_sys_descr[0..8]);
    appendTlv(*tmp, *tpos, 0x04, "CP-Router");
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // VB2: sysUpTime.0 (1.3.6.1.2.1.1.3.0) -> TimeTicks 0x00BC614E (12345678)
    const oid_uptime: [8]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x03, 0x00];
    const ticks_val: [4]u8 = [0x00, 0xBC, 0x61, 0x4E];
    tpos = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_uptime[0..8]);
    appendTlv(*tmp, *tpos, 0x43, ticks_val[0..4]);
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // VB3: ifHCInOctets.1 (1.3.6.1.2.1.31.1.1.1.6.1) -> Counter64 (9 字节带前导 0x00: 0x8000000102030405)
    const oid_hc_in: [11]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 31, 1, 1, 1, 6, 1];
    const c64_val: [9]u8 = [0x00, 0x80, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05];
    tpos = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_hc_in[0..11]);
    appendTlv(*tmp, *tpos, 0x46, c64_val[0..9]);
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // VB4: ipAdEntAddr -> IpAddress 10.20.30.40
    const oid_ip: [9]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 4, 20, 1, 1];
    const ip_val: [4]u8 = [10, 20, 30, 40];
    tpos = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_ip[0..9]);
    appendTlv(*tmp, *tpos, 0x40, ip_val[0..4]);
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // VB5: endOfMibView (0x82, len=0)
    const empty_bytes: [0]u8 = [];
    tpos = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_sys_descr[0..8]);
    appendTlv(*tmp, *tpos, 0x82, empty_bytes[0..0]);
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // VB6: INTEGER -42 (0xD6)
    const neg_val: [1]u8 = [0xD6];
    tpos = 0;
    appendTlv(*tmp, *tpos, 0x06, oid_sys_descr[0..8]);
    appendTlv(*tmp, *tpos, 0x02, neg_val[0..1]);
    appendTlv(*vbl, *vbl_pos, 0x30, tmp[0..tpos]);

    // 组装 PDU (0xA2 GetResponse)
    let pdu_buf: [320]u8 = [0; 320];
    let pdu: []u8 = pdu_buf[0..320];
    let pdu_pos: usize = 0;
    const req_id_raw: [2]u8 = [0x03, 0xE9];
    const zero_raw: [1]u8 = [0x00];
    appendTlv(*pdu, *pdu_pos, 0x02, req_id_raw[0..2]);
    appendTlv(*pdu, *pdu_pos, 0x02, zero_raw[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x02, zero_raw[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x30, vbl[0..vbl_pos]);

    // 组装外层报文
    let msg_body_buf: [360]u8 = [0; 360];
    let msg_body: []u8 = msg_body_buf[0..360];
    let mb_pos: usize = 0;
    const ver_v2c: [1]u8 = [0x01];
    appendTlv(*msg_body, *mb_pos, 0x02, ver_v2c[0..1]);
    appendTlv(*msg_body, *mb_pos, 0x04, "public");
    appendTlv(*msg_body, *mb_pos, 0xA2, pdu[0..pdu_pos]);

    let pkt_buf: [400]u8 = [0; 400];
    let pkt: []u8 = pkt_buf[0..400];
    let pkt_pos: usize = 0;
    appendTlv(*pkt, *pkt_pos, 0x30, msg_body[0..mb_pos]);

    const resp = parseSnmpMessage(pkt[0..pkt_pos]) ?? unreachable;
    expect_eq(resp.version, SNMP_V2C);
    expect_eq(resp.pdu_type == .get_response, true);
    expect_eq(resp.request_id, 1001);

    let it = resp.varbinds();
    let oid_out_buf: [64]u8 = [0; 64];
    let oid_out: []u8 = oid_out_buf[0..64];

    // VB1
    const vb1 = it.next() ?? unreachable;
    const n1 = formatSnmpOid(vb1.oid_bytes, *oid_out) ?? unreachable;
    expect_eq(sliceEq(oid_out[0..n1], "1.3.6.1.2.1.1.1.0"), true);
    const s1 = vb1.asOctetString() ?? unreachable;
    expect_eq(sliceEq(s1, "CP-Router"), true);
    expect_eq(vb1.asI64() == null, true);

    // VB2
    const vb2 = it.next() ?? unreachable;
    expect_eq(vb2.value_tag, SnmpValueTag.timeticks as u8);
    expect_eq(vb2.asU64() ?? 0, 12345678);

    // VB3
    const vb3 = it.next() ?? unreachable;
    expect_eq(vb3.value_tag, SnmpValueTag.counter64 as u8);
    expect_eq(vb3.asU64() ?? 0, 0x8000000102030405);

    // VB4
    const vb4 = it.next() ?? unreachable;
    const ip4 = vb4.asIp4() ?? unreachable;
    expect_eq(ip4[0], 10);
    expect_eq(ip4[1], 20);
    expect_eq(ip4[2], 30);
    expect_eq(ip4[3], 40);

    // VB5
    const vb5 = it.next() ?? unreachable;
    expect_eq(vb5.value_tag, SnmpValueTag.end_of_mib_view as u8);
    expect_eq(vb5.isException(), true);
    expect_eq(vb5.asU64() == null, true);

    // VB6
    const vb6 = it.next() ?? unreachable;
    expect_eq(vb6.asI64() ?? 0, -42);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. SNMP v1 Trap PDU (0xA4) with enterprise OID, agent_addr, generic/specific trap, timestamp & VarBind", () => {
    // enterprise: 1.3.6.1.4.1.8072.3.2.10 -> 8072 = (63 << 7) | 8 = [0xBF, 0x08]
    const ent_oid: [10]u8 = [0x2B, 0x06, 0x01, 0x04, 0x01, 0xBF, 0x08, 0x03, 0x02, 0x0A];
    const agent_ip: [4]u8 = [192, 168, 1, 254];
    const gen_trap: [1]u8 = [6];
    const spec_trap: [1]u8 = [42];
    const ts_raw: [3]u8 = [0x0F, 0x12, 0x06]; // 987654

    // 1 VarBind: Gauge32 = 85
    const vb_oid: [8]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00];
    const gauge_raw: [1]u8 = [85];
    let vb_inner_buf: [32]u8 = [0; 32];
    let vb_inner: []u8 = vb_inner_buf[0..32];
    let vi_pos: usize = 0;
    appendTlv(*vb_inner, *vi_pos, 0x06, vb_oid[0..8]);
    appendTlv(*vb_inner, *vi_pos, 0x42, gauge_raw[0..1]);

    let vbl_buf: [48]u8 = [0; 48];
    let vbl: []u8 = vbl_buf[0..48];
    let vbl_pos: usize = 0;
    appendTlv(*vbl, *vbl_pos, 0x30, vb_inner[0..vi_pos]);

    // Trap PDU body
    let pdu_buf: [128]u8 = [0; 128];
    let pdu: []u8 = pdu_buf[0..128];
    let pdu_pos: usize = 0;
    appendTlv(*pdu, *pdu_pos, 0x06, ent_oid[0..10]);
    appendTlv(*pdu, *pdu_pos, 0x40, agent_ip[0..4]);
    appendTlv(*pdu, *pdu_pos, 0x02, gen_trap[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x02, spec_trap[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x43, ts_raw[0..3]);
    appendTlv(*pdu, *pdu_pos, 0x30, vbl[0..vbl_pos]);

    let body_buf: [160]u8 = [0; 160];
    let body: []u8 = body_buf[0..160];
    let bpos: usize = 0;
    const ver_v1: [1]u8 = [0x00];
    appendTlv(*body, *bpos, 0x02, ver_v1[0..1]);
    appendTlv(*body, *bpos, 0x04, "trapcomm");
    appendTlv(*body, *bpos, 0xA4, pdu[0..pdu_pos]);

    let pkt_buf: [180]u8 = [0; 180];
    let pkt: []u8 = pkt_buf[0..180];
    let pkt_pos: usize = 0;
    appendTlv(*pkt, *pkt_pos, 0x30, body[0..bpos]);

    const trap = parseSnmpMessage(pkt[0..pkt_pos]) ?? unreachable;
    expect_eq(trap.version, SNMP_V1);
    expect_eq(sliceEq(trap.community, "trapcomm"), true);
    expect_eq(trap.pdu_type == .v1_trap, true);
    expect_eq(trap.agent_addr[0], 192);
    expect_eq(trap.agent_addr[1], 168);
    expect_eq(trap.agent_addr[2], 1);
    expect_eq(trap.agent_addr[3], 254);
    expect_eq(trap.generic_trap, 6);
    expect_eq(trap.specific_trap, 42);
    expect_eq(trap.timestamp, 987654);

    let oid_buf: [64]u8 = [0; 64];
    let oid_slice: []u8 = oid_buf[0..64];
    const ent_len = formatSnmpOid(trap.enterprise_oid, *oid_slice) ?? unreachable;
    expect_eq(sliceEq(oid_slice[0..ent_len], "1.3.6.1.4.1.8072.3.2.10"), true);

    let it = trap.varbinds();
    const vb = it.next() ?? unreachable;
    expect_eq(vb.value_tag, SnmpValueTag.gauge32 as u8);
    expect_eq(vb.asU64() ?? 0, 85);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. SNMP v2c GetBulkRequest (0xA5): non_repeaters & max_repetitions", () => {
    const bulk_pkt: [55]u8 = [
        0x30, 0x35, // SEQUENCE (53)
        0x02, 0x01, 0x01, // version = 1 (v2c)
        0x04, 0x06, 112, 117, 98, 108, 105, 99, // "public"
        0xA5, 0x28, // GetBulkRequest-PDU (40)
        0x02, 0x03, 0x7F, 0xFF, 0x01, // request-id = 8388353
        0x02, 0x01, 0x02, // non-repeaters = 2
        0x02, 0x01, 0x10, // max-repetitions = 16
        0x30, 0x1B, // VarBindList (27)
        0x30, 0x0C,
        0x06, 0x08, 0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00,
        0x05, 0x00,
        0x30, 0x0B,
        0x06, 0x07, 0x2B, 0x06, 0x01, 0x02, 0x01, 0x02, 0x02, // 1.3.6.1.2.1.2.2 (ifTable)
        0x05, 0x00,
    ];

    const msg = parseSnmpMessage(bulk_pkt[0..55]) ?? unreachable;
    expect_eq(msg.version, SNMP_V2C);
    expect_eq(msg.pdu_type == .get_bulk_request, true);
    expect_eq(msg.request_id, 8388353);
    expect_eq(msg.error_status, 2);
    expect_eq(msg.error_index, 16);
    expect_eq(msg.nonRepeaters(), 2);
    expect_eq(msg.maxRepetitions(), 16);

    let it = msg.varbinds();
    let count: usize = 0;
    while (vb := it.next()) {
        expect_eq(vb.isNull(), true);
        count += 1;
    }
    expect_eq(count, 2);
    expect_eq(it.had_error, false);
});

test("4. formatSnmpOid: classic MIB-2 OIDs, joint-iso-itu-t large first subid, and boundary guards", () => {
    let out_buf: [64]u8 = [0; 64];
    let out: []u8 = out_buf[0..64];

    // sysDescr.0 = 1.3.6.1.2.1.1.1.0
    const sys_descr: [8]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00];
    const n1 = formatSnmpOid(sys_descr[0..8], *out) ?? unreachable;
    expect_eq(sliceEq(out[0..n1], "1.3.6.1.2.1.1.1.0"), true);

    // 0.0
    const zero_oid: [1]u8 = [0x00];
    const n2 = formatSnmpOid(zero_oid[0..1], *out) ?? unreachable;
    expect_eq(sliceEq(out[0..n2], "0.0"), true);

    // 2.999.3 -> first_subid = 80 + 999 = 1079 = (8 << 7) | 55 = [0x88, 0x37]
    const large_arc2: [3]u8 = [0x88, 0x37, 0x03];
    const n3 = formatSnmpOid(large_arc2[0..3], *out) ?? unreachable;
    expect_eq(sliceEq(out[0..n3], "2.999.3"), true);

    // 非法输入：空切片
    const empty_oid: [0]u8 = [];
    expect_eq(formatSnmpOid(empty_oid[0..0], *out) == null, true);

    // 非法输入：截断的 VLQ（末尾字节高位置 1）
    const trunc_oid: [3]u8 = [0x2B, 0x06, 0x81];
    expect_eq(formatSnmpOid(trunc_oid[0..3], *out) == null, true);

    // 非法输入：非最小编码（子标识首字节为 0x80）
    const non_min_oid: [3]u8 = [0x2B, 0x80, 0x01];
    expect_eq(formatSnmpOid(non_min_oid[0..3], *out) == null, true);

    // 输出缓冲区过短（sysDescr.0 需要 17 字节，仅提供 10 字节）
    let small_slice: []u8 = out_buf[0..10];
    expect_eq(formatSnmpOid(sys_descr[0..8], *small_slice) == null, true);
});

test("5. Long-form BER length encoding (>127 bytes payload) & SNMPv2-Trap (0xA7)", () => {
    let payload: [140]u8 = [65; 140]; // 140 个 'A'，触发 0x81 0x8C 长形式长度
    const oid_sys_descr: [8]u8 = [0x2B, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00];

    let vb_inner_buf: [200]u8 = [0; 200];
    let vb_inner: []u8 = vb_inner_buf[0..200];
    let vi_pos: usize = 0;
    appendTlv(*vb_inner, *vi_pos, 0x06, oid_sys_descr[0..8]);
    appendTlv(*vb_inner, *vi_pos, 0x04, payload[0..140]);

    let vbl_buf: [220]u8 = [0; 220];
    let vbl: []u8 = vbl_buf[0..220];
    let vbl_pos: usize = 0;
    appendTlv(*vbl, *vbl_pos, 0x30, vb_inner[0..vi_pos]);

    let pdu_buf: [260]u8 = [0; 260];
    let pdu: []u8 = pdu_buf[0..260];
    let pdu_pos: usize = 0;
    const req_id: [1]u8 = [7];
    const zero: [1]u8 = [0];
    appendTlv(*pdu, *pdu_pos, 0x02, req_id[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x02, zero[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x02, zero[0..1]);
    appendTlv(*pdu, *pdu_pos, 0x30, vbl[0..vbl_pos]);

    let body_buf: [300]u8 = [0; 300];
    let body: []u8 = body_buf[0..300];
    let bpos: usize = 0;
    const v2c: [1]u8 = [1];
    appendTlv(*body, *bpos, 0x02, v2c[0..1]);
    appendTlv(*body, *bpos, 0x04, "notify");
    appendTlv(*body, *bpos, 0xA7, pdu[0..pdu_pos]); // SNMPv2-Trap

    let pkt_buf: [320]u8 = [0; 320];
    let pkt: []u8 = pkt_buf[0..320];
    let pkt_pos: usize = 0;
    appendTlv(*pkt, *pkt_pos, 0x30, body[0..bpos]);

    const msg = parseSnmpMessage(pkt[0..pkt_pos]) ?? unreachable;
    expect_eq(msg.version, SNMP_V2C);
    expect_eq(msg.pdu_type == .v2_trap, true);
    expect_eq(msg.request_id, 7);

    let it = msg.varbinds();
    const vb = it.next() ?? unreachable;
    const oct = vb.asOctetString() ?? unreachable;
    expect_eq(oct.len, 140);
    expect_eq(oct[0], 65);
    expect_eq(oct[139], 65);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("6. Malformed packets defense: bad version, truncated BER, indefinite length, trailing garbage, bad VarBind", () => {
    // 6.1 空数据
    const empty: [0]u8 = [];
    expect_eq(parseSnmpMessage(empty[0..0]) == null, true);

    // 6.2 非法版本 (SNMPv3 = 3)
    const bad_ver: [23]u8 = [
        0x30, 0x15,
        0x02, 0x01, 0x03, // version = 3 (unsupported in v1/v2c)
        0x04, 0x03, 112, 117, 98,
        0xA0, 0x0B,
        0x02, 0x01, 0x01,
        0x02, 0x01, 0x00,
        0x02, 0x01, 0x00,
        0x30, 0x00,
    ];
    expect_eq(parseSnmpMessage(bad_ver[0..23]) == null, true);

    // 6.3 截断 BER 报文（外层声明 21 字节但仅提供 18 字节）
    const truncated: [18]u8 = [
        0x30, 0x15,
        0x02, 0x01, 0x01,
        0x04, 0x03, 112, 117, 98,
        0xA0, 0x0B,
        0x02, 0x01, 0x01,
        0x02, 0x01, 0x00,
    ];
    expect_eq(parseSnmpMessage(truncated[0..18]) == null, true);

    // 6.4 不定长度 (0x80) 必须拒绝
    const indef: [6]u8 = [0x30, 0x80, 0x02, 0x01, 0x00, 0x00];
    expect_eq(parseSnmpMessage(indef[0..6]) == null, true);

    // 6.5 尾随垃圾字节必须拒绝（合法 23 字节报文后多出 0xFF）
    const trailing: [24]u8 = [
        0x30, 0x15, // len = 21 -> total = 23
        0x02, 0x01, 0x01,
        0x04, 0x03, 112, 117, 98,
        0xA0, 0x0B,
        0x02, 0x01, 0x01,
        0x02, 0x01, 0x00,
        0x02, 0x01, 0x00,
        0x30, 0x00,
        0xFF, // trailing garbage
    ];
    expect_eq(parseSnmpMessage(trailing[0..24]) == null, true);

    // 6.6 合法 23 字节基线报文应当成功
    const valid_min = parseSnmpMessage(trailing[0..23]) ?? unreachable;
    expect_eq(valid_min.version, SNMP_V2C);
    expect_eq(valid_min.request_id, 1);

    // 6.7 VarBindIter 遇到畸形 VarBind（截断的 SEQUENCE）时置 had_error = true 并终止
    const bad_vbl: [5]u8 = [0x30, 0x0A, 0x06, 0x02, 0x2B];
    let bad_it = SnmpVarBindIter.init(bad_vbl[0..5]);
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.had_error, true);
});
