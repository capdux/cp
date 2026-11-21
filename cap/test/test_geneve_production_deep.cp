// Source: cp/cap/test/test_geneve_production_deep.cp (Production deep tests for cap:geneve)

import { test, expect_eq } from "cap:test";
import {
    GENEVE_MIN_HEADER_SIZE,
    GENEVE_DEFAULT_PORT,
    GENEVE_VERSION,
    GENEVE_MAX_VNI,
    GENEVE_FLAG_OAM,
    GENEVE_FLAG_CRITICAL,
    GENEVE_OPT_CRITICAL_BIT,
    PROTO_ETHERNET,
    PROTO_IPV4,
    PROTO_IPV6,
    GeneveHeader,
    GeneveOption,
    GeneveOptionIter,
    GenevePacket,
    GeneveWriter,
    isGeneve,
    parseGeneve,
} from "cap:geneve";

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

test("1. No-option Geneve roundtrip with 24-bit VNI & PROTO_ETHERNET / PROTO_IPV4 / PROTO_IPV6", () => {
    expect_eq(GENEVE_MIN_HEADER_SIZE, 8);
    expect_eq(GENEVE_DEFAULT_PORT, 6081);
    expect_eq(GENEVE_VERSION, 0);
    expect_eq(GENEVE_MAX_VNI, 0x00FFFFFF);

    let buf: [128]u8 = [0; 128];
    let w1 = GeneveWriter.init(buf[0..128], PROTO_ETHERNET, 0x00ABCDEF, false) ?? unreachable;
    const wire1 = w1.finish("eth-frame-payload") ?? unreachable;
    expect_eq(wire1.len, 8 + 17);
    expect_eq(isGeneve(wire1), true);

    const p1 = parseGeneve(wire1) ?? unreachable;
    expect_eq(p1.header.version, 0);
    expect_eq(p1.header.opt_len_words, 0);
    expect_eq(p1.header.isOam(), false);
    expect_eq(p1.header.hasCriticalOptions(), false);
    expect_eq(p1.header.protocol_type, PROTO_ETHERNET);
    expect_eq(p1.header.vni, 0x00ABCDEF);
    expect_eq(p1.header.reserved, 0);
    expect_eq(p1.headerBytes(), 8);
    expect_eq(p1.options_slice.len, 0);
    expect_eq(sliceEq(p1.payload, "eth-frame-payload"), true);

    // IPv4 & IPv6 protocol types with boundary VNIs (0 and 0x00FFFFFF)
    let w2 = GeneveWriter.init(buf[0..128], PROTO_IPV4, 0, false) ?? unreachable;
    const wire2 = w2.finish("v4") ?? unreachable;
    const p2 = parseGeneve(wire2) ?? unreachable;
    expect_eq(p2.header.protocol_type, PROTO_IPV4);
    expect_eq(p2.header.vni, 0);

    let w3 = GeneveWriter.init(buf[0..128], PROTO_IPV6, GENEVE_MAX_VNI, false) ?? unreachable;
    const wire3 = w3.finish("v6") ?? unreachable;
    const p3 = parseGeneve(wire3) ?? unreachable;
    expect_eq(p3.header.protocol_type, PROTO_IPV6);
    expect_eq(p3.header.vni, 0x00FFFFFF);
});

test("2. Multi-TLV options roundtrip with Critical option auto-setting C-flag & GeneveOptionIter", () => {
    let buf: [256]u8 = [0; 256];
    let w = GeneveWriter.init(buf[0..256], PROTO_ETHERNET, 4096, false) ?? unreachable;

    // Opt 1: 非关键选项，class=0x0102 (Linux), type=0x01, 4 字节数据
    const opt1_data: [4]u8 = [0x11, 0x22, 0x33, 0x44];
    expect_eq(w.writeOption(0x0102, 0x01, opt1_data[0..4]), true);

    // Opt 2: 关键选项 (bit 7 = 1)，class=0x0107 (VMware), type=0x80|0x09, 8 字节数据 -> 自动置位 C-flag
    const opt2_data: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    expect_eq(w.writeOption(0x0107, GENEVE_OPT_CRITICAL_BIT | 0x09, opt2_data[0..8]), true);

    // Opt 3: 零长度选项数据（仅 4B 选项头），class=0xFFFF, type=0x03
    const empty_data: [0]u8 = [];
    expect_eq(w.writeOption(0xFFFF, 0x03, empty_data[0..0]), true);

    const wire = w.finish("inner-data") ?? unreachable;
    // 总选项字节数 = (4+4) + (4+8) + (4+0) = 24 字节 = 6 words
    const pkt = parseGeneve(wire) ?? unreachable;
    expect_eq(pkt.header.opt_len_words, 6);
    expect_eq(pkt.headerBytes(), 8 + 24);
    expect_eq(pkt.header.hasCriticalOptions(), true);
    expect_eq(pkt.header.isOam(), false);
    expect_eq(pkt.options_slice.len, 24);
    expect_eq(sliceEq(pkt.payload, "inner-data"), true);

    let it = pkt.options();

    const o1 = it.next() ?? unreachable;
    expect_eq(o1.opt_class, 0x0102);
    expect_eq(o1.isCritical(), false);
    expect_eq(o1.typeWithoutCritical(), 0x01);
    expect_eq(o1.len_words, 1);
    expect_eq(sliceEq(o1.data, opt1_data[0..4]), true);

    const o2 = it.next() ?? unreachable;
    expect_eq(o2.opt_class, 0x0107);
    expect_eq(o2.isCritical(), true);
    expect_eq(o2.typeWithoutCritical(), 0x09);
    expect_eq(o2.len_words, 2);
    expect_eq(sliceEq(o2.data, opt2_data[0..8]), true);

    const o3 = it.next() ?? unreachable;
    expect_eq(o3.opt_class, 0xFFFF);
    expect_eq(o3.isCritical(), false);
    expect_eq(o3.typeWithoutCritical(), 0x03);
    expect_eq(o3.len_words, 0);
    expect_eq(o3.data.len, 0);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. OAM control packet flag with empty options & empty payload", () => {
    let buf: [64]u8 = [0; 64];
    let w = GeneveWriter.init(buf[0..64], PROTO_ETHERNET, 0x001020, true) ?? unreachable;
    const empty_payload: [0]u8 = [];
    const wire = w.finish(empty_payload[0..0]) ?? unreachable;
    expect_eq(wire.len, 8);

    const pkt = parseGeneve(wire) ?? unreachable;
    expect_eq(pkt.header.isOam(), true);
    expect_eq(pkt.header.hasCriticalOptions(), false);
    expect_eq(pkt.header.flags, GENEVE_FLAG_OAM);
    expect_eq(pkt.header.vni, 0x001020);
    expect_eq(pkt.options_slice.len, 0);
    expect_eq(pkt.payload.len, 0);

    let it = pkt.options();
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("4. Writer boundary enforcement: >24-bit VNI, non-4B option, >252B options limit, small buffer", () => {
    let buf: [300]u8 = [0; 300];

    // 4.1 超过 24-bit 的 VNI 拒绝初始化
    expect_eq(GeneveWriter.init(buf[0..300], PROTO_IPV4, 0x01000000, false) == null, true);

    // 4.2 缓冲区小于 8 字节拒绝初始化
    expect_eq(GeneveWriter.init(buf[0..7], PROTO_IPV4, 1, false) == null, true);

    // 4.3 非 4 字节倍数的选项数据必须拒绝
    let w = GeneveWriter.init(buf[0..300], PROTO_IPV4, 100, false) ?? unreachable;
    const bad_opt3: [3]u8 = [1, 2, 3];
    expect_eq(w.writeOption(0x0102, 0x01, bad_opt3[0..3]), false);

    // 4.4 选项总长度精确上限 252 字节：写入 2 个 124B 选项（共 128 + 128 = 256 > 252）第二条必失败
    const big124: [124]u8 = [0xAA; 124];
    expect_eq(w.writeOption(0x0102, 0x01, big124[0..124]), true); // 128B
    expect_eq(w.writeOption(0x0102, 0x02, big124[0..124]), false); // 128+128=256 > 252

    // 改为写入 120B 选项（4+120=124B），总计 128+124 = 252B 精确达标！
    const exact120: [120]u8 = [0xBB; 120];
    expect_eq(w.writeOption(0x0102, 0x02, exact120[0..120]), true);
    // 再多写哪怕 0B 数据（4B 头）也超 252B，必失败
    const zero_d: [0]u8 = [];
    expect_eq(w.writeOption(0x0102, 0x03, zero_d[0..0]), false);

    const wire = w.finish("ok") ?? unreachable;
    const pkt = parseGeneve(wire) ?? unreachable;
    expect_eq(pkt.header.opt_len_words, 63);
    expect_eq(pkt.headerBytes(), 8 + 252);
    expect_eq(sliceEq(pkt.payload, "ok"), true);
});

test("5. Malformed packet & option iterator defense: non-zero version, truncated opt_len, non-zero opt reserved bits", () => {
    // 5.1 短于 8 字节
    const short_pkt: [7]u8 = [0; 7];
    expect_eq(isGeneve(short_pkt[0..7]), false);
    expect_eq(parseGeneve(short_pkt[0..7]) == null, true);

    // 5.2 非零 version (version = 1 -> b0 = 0x40)
    const bad_ver: [8]u8 = [0x40, 0x00, 0x65, 0x58, 0x00, 0x00, 0x01, 0x00];
    expect_eq(isGeneve(bad_ver[0..8]), false);
    expect_eq(parseGeneve(bad_ver[0..8]) == null, true);

    // 5.3 opt_len_words 越界（声明 4 words = 16B，总需 24B，实际仅 12B）
    const oob_opt: [12]u8 = [0x04, 0x00, 0x65, 0x58, 0x00, 0x00, 0x01, 0x00, 0, 0, 0, 0];
    expect_eq(isGeneve(oob_opt[0..12]), false);
    expect_eq(parseGeneve(oob_opt[0..12]) == null, true);

    // 5.4 OptionIter 遇到非零 3-bit 保留位 (b3 = 0x20 | 0x01) -> had_error = true
    const bad_rsvd_opt: [8]u8 = [0x01, 0x02, 0x01, 0x21, 0xAA, 0xBB, 0xCC, 0xDD];
    let it1 = GeneveOptionIter.init(bad_rsvd_opt[0..8]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // 5.5 OptionIter 遇到截断的选项数据（声明 len_words = 2 -> 8B，实际仅 4B） -> had_error = true
    const trunc_opt: [8]u8 = [0x01, 0x02, 0x01, 0x02, 0xAA, 0xBB, 0xCC, 0xDD];
    let it2 = GeneveOptionIter.init(trunc_opt[0..8]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);
});
