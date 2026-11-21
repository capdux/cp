// Source: cp/cap/test/test_isis_production_deep.cp (Production deep tests for cap:isis)

import { test, expect_eq } from "cap:test";
import {
    ISIS_NLPID,
    ISIS_COMMON_HEADER_SIZE,
    ISIS_VERSION,
    ISIS_LSP_HEADER_SIZE,
    PDU_L1_LAN_IIH,
    PDU_L2_LAN_IIH,
    PDU_P2P_IIH,
    PDU_L1_LSP,
    PDU_L2_LSP,
    PDU_L1_CSNP,
    PDU_L2_CSNP,
    PDU_L1_PSNP,
    PDU_L2_PSNP,
    TLV_AREA_ADDR,
    TLV_IS_NEIGHBORS,
    TLV_PADDING,
    TLV_AUTH,
    TLV_EXT_IS_REACH,
    TLV_PROTOCOLS,
    TLV_IP_INTF_ADDR,
    TLV_EXT_IP_REACH,
    TLV_HOSTNAME,
    IsisCommonHeader,
    IsisLspHeader,
    IsisTlv,
    IsisTlvIter,
    IsisPacket,
    isisFletcher16,
    isisLspChecksum,
    verifyIsisLspChecksum,
    isIsis,
    parseIsis,
    parseIsisLsp,
} from "cap:isis";

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

test("1. L2 LSP full parsing with Hostname, Protocols & IP Interface TLVs + Fletcher-16 verification", () => {
    expect_eq(ISIS_NLPID, 0x83);
    expect_eq(ISIS_COMMON_HEADER_SIZE, 8);
    expect_eq(ISIS_VERSION, 1);
    expect_eq(ISIS_LSP_HEADER_SIZE, 27);

    // 27B LSP header + TLV 129 (3B) + TLV 132 (6B) + TLV 137 "core-rtr1" (11B) = 47 bytes
    let raw: [47]u8 = [0; 47];
    // Common header (0..8)
    raw[0] = ISIS_NLPID;
    raw[1] = 27;          // len_indicator
    raw[2] = ISIS_VERSION;
    raw[3] = 0;           // id_len = 0 -> default 6B
    raw[4] = PDU_L2_LSP;  // 20
    raw[5] = ISIS_VERSION;
    raw[6] = 0;           // reserved
    raw[7] = 3;           // max_area_addrs

    // LSP fixed body (8..27)
    raw[8] = 0; raw[9] = 47;          // pdu_len = 47
    raw[10] = 0x04; raw[11] = 0xB0;   // remaining_lifetime = 1200s
    // lsp_id (12..20): 1921.6800.1001.00-00
    raw[12] = 0x19; raw[13] = 0x21; raw[14] = 0x68; raw[15] = 0x00;
    raw[16] = 0x10; raw[17] = 0x01; raw[18] = 0x00; raw[19] = 0x00;
    // seq_num = 0x00000015 (21)
    raw[20] = 0; raw[21] = 0; raw[22] = 0; raw[23] = 21;
    // checksum @ 24..26 (computed below)
    // type_block @ 26: Overload (0x04) | IS Type L1/L2 (0x03) = 0x07
    raw[26] = 0x07;

    // TLV 129 (Protocols Supported): IPv4 (0xCC)
    raw[27] = TLV_PROTOCOLS;
    raw[28] = 1;
    raw[29] = 0xCC;

    // TLV 132 (IP Interface Address): 10.255.0.1
    raw[30] = TLV_IP_INTF_ADDR;
    raw[31] = 4;
    raw[32] = 10; raw[33] = 255; raw[34] = 0; raw[35] = 1;

    // TLV 137 (Dynamic Hostname): "core-rtr1" (9 bytes)
    raw[36] = TLV_HOSTNAME;
    raw[37] = 9;
    raw[38] = 99; raw[39] = 111; raw[40] = 114; raw[41] = 101;
    raw[42] = 45; raw[43] = 114; raw[44] = 116; raw[45] = 114; raw[46] = 49;

    // 计算并填入 ISO 8473 Fletcher-16 校验和（从偏移 12 开始）
    const csum: u16 = isisLspChecksum(raw[12..47]);
    raw[24] = ((csum >> 8) & 0xFF) as u8;
    raw[25] = (csum & 0xFF) as u8;

    expect_eq(verifyIsisLspChecksum(raw[12..47]), true);
    expect_eq(isisFletcher16(raw[12..47]), 0);

    expect_eq(isIsis(raw[0..47]), true);
    const pkt = parseIsis(raw[0..47]) ?? unreachable;
    expect_eq(pkt.common.systemIdLen(), 6);
    expect_eq(pkt.common.pdu_type, PDU_L2_LSP);

    const lsp = parseIsisLsp(pkt) ?? unreachable;
    expect_eq(lsp.pdu_len, 47);
    expect_eq(lsp.remaining_lifetime, 1200);
    expect_eq(lsp.lsp_id[0], 0x19);
    expect_eq(lsp.lsp_id[5], 0x01);
    expect_eq(lsp.seq_num, 21);
    expect_eq(lsp.checksum, csum);
    expect_eq(lsp.isPartitionRepair(), false);
    expect_eq(lsp.isOverload(), true);
    expect_eq(lsp.isType(), 3);

    // Hostname & TLV 查找
    const host = pkt.hostname() ?? unreachable;
    expect_eq(sliceEq(host, "core-rtr1"), true);

    const proto_tlv = pkt.findTlv(TLV_PROTOCOLS) ?? unreachable;
    expect_eq(proto_tlv.length, 1);
    expect_eq(proto_tlv.value[0], 0xCC);

    const ip_tlv = pkt.findTlv(TLV_IP_INTF_ADDR) ?? unreachable;
    expect_eq(ip_tlv.length, 4);
    expect_eq(ip_tlv.value[0], 10);
    expect_eq(ip_tlv.value[3], 1);

    // 完整迭代校验
    let it = pkt.tlvs();
    let count: usize = 0;
    while (t := it.next()) {
        * = t.tlv_type;
        count += 1;
    }
    expect_eq(count, 3);
    expect_eq(it.had_error, false);
});

test("2. L1 LSP Partition Repair bit & Fletcher-16 tamper detection", () => {
    let raw: [30]u8 = [0; 30];
    raw[0] = ISIS_NLPID;
    raw[1] = 27;
    raw[2] = 1;
    raw[3] = 6; // explicit 6B system ID
    raw[4] = PDU_L1_LSP;
    raw[5] = 1;
    raw[8] = 0; raw[9] = 30;
    raw[26] = 0x81; // P-bit (0x80) | IS Type L1 (0x01)
    raw[27] = TLV_PROTOCOLS;
    raw[28] = 1;
    raw[29] = 0x8E; // IPv6

    const csum = isisLspChecksum(raw[12..30]);
    raw[24] = ((csum >> 8) & 0xFF) as u8;
    raw[25] = (csum & 0xFF) as u8;
    expect_eq(verifyIsisLspChecksum(raw[12..30]), true);

    const pkt = parseIsis(raw[0..30]) ?? unreachable;
    expect_eq(pkt.common.systemIdLen(), 6);
    const lsp = parseIsisLsp(pkt) ?? unreachable;
    expect_eq(lsp.isPartitionRepair(), true);
    expect_eq(lsp.isOverload(), false);
    expect_eq(lsp.isType(), 1);

    // 篡改单字节后 Fletcher-16 校验必须失败
    raw[29] = 0x8F;
    expect_eq(verifyIsisLspChecksum(raw[12..30]), false);
    expect_eq(isisFletcher16(raw[12..30]) != 0, true);
});

test("3. P2P IIH (PDU 17) general parsing with Area Address TLV", () => {
    // P2P IIH fixed header = 20B (8B common + 12B fixed body) + 6B Area TLV = 26B
    let raw: [26]u8 = [0; 26];
    raw[0] = ISIS_NLPID;
    raw[1] = 20;
    raw[2] = 1;
    raw[3] = 0;
    raw[4] = PDU_P2P_IIH;
    raw[5] = 1;
    raw[6] = 0;
    raw[7] = 0;

    // Area Address TLV (code 1, len 4): area_len=3, 49.0001
    raw[20] = TLV_AREA_ADDR;
    raw[21] = 4;
    raw[22] = 3;
    raw[23] = 0x49; raw[24] = 0x00; raw[25] = 0x01;

    const pkt = parseIsis(raw[0..26]) ?? unreachable;
    expect_eq(pkt.common.pdu_type, PDU_P2P_IIH);
    expect_eq(pkt.fixed_body.len, 12);
    expect_eq(pkt.tlv_bytes.len, 6);
    expect_eq(pkt.hostname() == null, true);

    // 非 LSP 报文传入 parseIsisLsp 必须返回 null
    expect_eq(parseIsisLsp(pkt) == null, true);

    const area = pkt.findTlv(TLV_AREA_ADDR) ?? unreachable;
    expect_eq(area.length, 4);
    expect_eq(area.value[1], 0x49);
});

test("4. Fletcher-16 deterministic digest on arbitrary byte sequences", () => {
    const d1: [5]u8 = [1, 2, 3, 4, 5];
    // c0 = 15, c1 = 1 + 3 + 6 + 10 + 15 = 35 -> (35 << 8) | 15 = 0x230F
    expect_eq(isisFletcher16(d1[0..5]), 0x230F);

    const short10: [10]u8 = [0; 10];
    expect_eq(verifyIsisLspChecksum(short10[0..10]), false);
    expect_eq(isisLspChecksum(short10[0..10]), 0);
});

test("5. Malformed IS-IS defense: bad NLPID/version/reserved, len_indicator OOB, id_len > 8, truncated TLV", () => {
    // 5.1 短于 8B
    const short7: [7]u8 = [0x83, 8, 1, 0, 17, 1, 0];
    expect_eq(isIsis(short7[0..7]), false);
    expect_eq(parseIsis(short7[0..7]) == null, true);

    // 5.2 非法 NLPID (0x81 CLNP)
    let bad: [27]u8 = [0; 27];
    bad[0] = 0x81; bad[1] = 27; bad[2] = 1; bad[4] = PDU_L1_LSP; bad[5] = 1;
    expect_eq(parseIsis(bad[0..27]) == null, true);

    // 5.3 非法 version 或非零 reserved
    bad[0] = ISIS_NLPID; bad[2] = 2;
    expect_eq(parseIsis(bad[0..27]) == null, true);
    bad[2] = 1; bad[6] = 1;
    expect_eq(parseIsis(bad[0..27]) == null, true);
    bad[6] = 0;

    // 5.4 len_indicator 越界 (< 8 或 > data.len)
    bad[1] = 7;
    expect_eq(parseIsis(bad[0..27]) == null, true);
    bad[1] = 30;
    expect_eq(parseIsis(bad[0..27]) == null, true);
    bad[1] = 27;

    // 5.5 id_len > 8 拒绝
    bad[3] = 9;
    expect_eq(parseIsis(bad[0..27]) == null, true);
    bad[3] = 0;

    // 5.6 LSP pdu_len > raw_len 拒绝
    bad[8] = 0; bad[9] = 40;
    const pkt_oob = parseIsis(bad[0..27]) ?? unreachable;
    expect_eq(parseIsisLsp(pkt_oob) == null, true);

    // 5.7 截断 TLV 迭代置 had_error = true
    const trunc_tlv: [4]u8 = [TLV_HOSTNAME, 10, 65, 66];
    let it = IsisTlvIter.init(trunc_tlv[0..4]);
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, true);
});
