// Source: cp/cap/test/test_pim_production_deep.cp (Production deep tests for cap:pim)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PIM_IP_PROTO,
    PIM_VERSION,
    PIM_HEADER_SIZE,
    PIM_HELLO_OPT_HEADER_SIZE,
    PIM_HELLO,
    PIM_REGISTER,
    PIM_REGISTER_STOP,
    PIM_JOIN_PRUNE,
    PIM_BOOTSTRAP,
    PIM_ASSERT,
    PIM_GRAFT,
    PIM_GRAFT_ACK,
    PIM_CANDIDATE_RP_ADV,
    OPT_HOLDTIME,
    OPT_LAN_PRUNE_DELAY,
    OPT_DR_PRIORITY,
    OPT_GENERATION_ID,
    OPT_ADDRESS_LIST,
    REG_FLAG_BORDER,
    REG_FLAG_NULL,
    pimChecksumIpv4,
    verifyPimChecksumIpv4,
    pimChecksumIpv6,
    verifyPimChecksumIpv6,
    PimHeader,
    PimHelloOption,
    PimHelloOptIter,
    PimPacket,
    PimRegister,
    parsePimRegister,
    isPim,
    parsePim,
    PimWriter,
} from "cap:iot";

test("1. PIM Hello multi-option TLV iteration and IPv4 checksum roundtrip", () => {
    expect_eq(PIM_IP_PROTO, 103 as u8);
    expect_eq(PIM_VERSION, 2 as u8);
    expect_eq(PIM_HEADER_SIZE, 4);
    expect_eq(PIM_HELLO_OPT_HEADER_SIZE, 4);

    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PimWriter.init(buf[0..], PIM_HELLO);
    expect_eq(w.writeHoldtime(105), true);
    const lan_delay: [4]u8 = [0x01, 0xF4, 0x09, 0xC4]; // 500ms prop, 2500ms override
    expect_eq(w.writeHelloOption(OPT_LAN_PRUNE_DELAY, lan_delay[0..]), true);
    expect_eq(w.writeDrPriority(1000), true);
    expect_eq(w.writeGenerationId(0xAABB_CCDD), true);
    const addr_list: [6]u8 = [1, 0, 10, 0, 0, 1];
    expect_eq(w.writeHelloOption(OPT_ADDRESS_LIST, addr_list[0..]), true);

    const wire = w.finishIpv4() ?? unreachable;
    expect_eq(isPim(wire), true);
    expect_eq(verifyPimChecksumIpv4(wire), true);

    const pkt = parsePim(wire) ?? unreachable;
    expect_eq(pkt.header.version, PIM_VERSION);
    expect_eq(pkt.header.msg_type, PIM_HELLO);
    expect_eq(pkt.header.checksum, pimChecksumIpv4(wire));
    expect_eq(pkt.holdtime() ?? 0, 105 as u16);
    expect_eq(pkt.drPriority() ?? 0, 1000 as u32);
    expect_eq(pkt.generationId() ?? 0, 0xAABB_CCDD as u32);

    const opt_lan = pkt.findHelloOption(OPT_LAN_PRUNE_DELAY) ?? unreachable;
    expect_eq(opt_lan.opt_len, 4 as u16);
    const opt_addr = pkt.findHelloOption(OPT_ADDRESS_LIST) ?? unreachable;
    expect_eq(opt_addr.opt_len, 6 as u16);

    let it = pkt.helloOptions();
    let count: usize = 0;
    while (true) {
        if (o := it.next()) {
            * = o;
            count += 1;
        } else {
            break;
        }
    }
    expect_eq(count, 5);
    expect_eq(it.hasError(), false);
});

test("2. PIM Register flags (Border / Null-Register), inner packet extraction, and 8B checksum special case", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PimWriter.init(buf[0..], PIM_REGISTER);
    const inner_ip: [8]u8 = [0x45, 0x00, 0x00, 0x1C, 0x12, 0x34, 0x00, 0x00];
    expect_eq(w.writeRegister(REG_FLAG_BORDER | REG_FLAG_NULL, inner_ip[0..]), true);
    const wire = w.finishIpv4() ?? unreachable;
    const total = wire.len;
    expect_eq(total, 16);
    expect_eq(verifyPimChecksumIpv4(buf[0..total]), true);

    const pkt = parsePim(buf[0..total]) ?? unreachable;
    const reg = parsePimRegister(pkt.body) ?? unreachable;
    expect_eq(reg.isBorder(), true);
    expect_eq(reg.isNullRegister(), true);
    expect_eq(std.mem.eql(u8, reg.inner_packet, inner_ip[0..]), true);

    // RFC 7761 Section 4.9 special rule: Register checksum ONLY covers first 8 bytes!
    // Modifying byte 10 (inside inner_packet) must NOT invalidate PIM Register checksum.
    buf[10] = buf[10] ^ 0xFF;
    expect_eq(verifyPimChecksumIpv4(buf[0..total]), true);

    // Modifying byte 4 (inside Register flags, within first 8 bytes) MUST invalidate checksum.
    buf[4] = buf[4] ^ 0x01;
    expect_eq(verifyPimChecksumIpv4(buf[0..total]), false);
});

test("3. PIM over IPv6 40-byte pseudo-header checksum verification (Hello and Register)", () => {
    const src_v6: [16]u8 = [0xFE, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const dst_v6: [16]u8 = [0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0D]; // ff02::d All-PIM-Routers

    // Hello over IPv6
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = PimWriter.init(buf[0..], PIM_HELLO);
    expect_eq(w.writeHoldtime(105), true);
    expect_eq(w.writeDrPriority(50), true);
    const wire = w.finishIpv6(src_v6, dst_v6) ?? unreachable;
    expect_eq(verifyPimChecksumIpv6(src_v6, dst_v6, wire), true);

    // Changing destination IPv6 address must fail verification
    let bad_dst = dst_v6;
    bad_dst[15] = 0x0E;
    expect_eq(verifyPimChecksumIpv6(src_v6, bad_dst, wire), false);

    // Register over IPv6 (also covers only first 8 bytes + pseudo-header with length 8)
    let rbuf: [64]u8 = std.mem.zeroes([64]u8);
    let rw = PimWriter.init(rbuf[0..], PIM_REGISTER);
    const inner_v6: [4]u8 = [0x60, 0x00, 0x00, 0x00];
    expect_eq(rw.writeRegister(REG_FLAG_NULL, inner_v6[0..]), true);
    const rwire = rw.finishIpv6(src_v6, dst_v6) ?? unreachable;
    expect_eq(verifyPimChecksumIpv6(src_v6, dst_v6, rwire), true);
});

test("4. All PIM v2 message types (0..8) accepted and classified", () => {
    const types: [9]u8 = [
        PIM_HELLO,
        PIM_REGISTER,
        PIM_REGISTER_STOP,
        PIM_JOIN_PRUNE,
        PIM_BOOTSTRAP,
        PIM_ASSERT,
        PIM_GRAFT,
        PIM_GRAFT_ACK,
        PIM_CANDIDATE_RP_ADV,
    ];
    let i: usize = 0;
    while (i < 9) {
        let buf: [16]u8 = std.mem.zeroes([16]u8);
        let w = PimWriter.init(buf[0..], types[i]);
        if (types[i] == PIM_REGISTER) {
            expect_eq(w.writeRegister(0, ""), true);
        }
        const wire = w.finishIpv4() ?? unreachable;
        const pkt = parsePim(wire) ?? unreachable;
        expect_eq(pkt.header.msg_type, types[i]);
        expect_eq(verifyPimChecksumIpv4(wire), true);
        i += 1;
    }
});

test("5. Invalid version, out-of-range msg_type, truncated Register, and Hello TLV overflow rejection", () => {
    // 1. Shorter than 4B
    const short3: [3]u8 = [0x20, 0, 0];
    expect_eq(isPim(short3[0..]), false);
    expect_eq(parsePim(short3[0..]) == null, true);
    expect_eq(verifyPimChecksumIpv4(short3[0..]), false);

    // 2. Wrong version (v1 = 0x10, v3 = 0x30)
    const v1: [4]u8 = [0x10, 0, 0, 0];
    const v3: [4]u8 = [0x30, 0, 0, 0];
    expect_eq(isPim(v1[0..]), false);
    expect_eq(parsePim(v1[0..]) == null, true);
    expect_eq(isPim(v3[0..]), false);
    expect_eq(parsePim(v3[0..]) == null, true);

    // 3. Invalid msg_type (9 > 8)
    const bad_type: [4]u8 = [0x29, 0, 0, 0];
    expect_eq(isPim(bad_type[0..]), false);
    expect_eq(parsePim(bad_type[0..]) == null, true);

    // 4. Truncated Register (only 4B header, missing 4B flags)
    const trunc_reg: [4]u8 = [0x21, 0, 0, 0];
    expect_eq(parsePim(trunc_reg[0..]) == null, true);
    expect_eq(parsePimRegister(trunc_reg[0..3]) == null, true);

    // 5. Hello TLV claiming opt_len=10 when only 2 bytes follow
    const oob_hello: [10]u8 = [0x20, 0, 0, 0, 0x00, 0x01, 0x00, 10, 0x00, 0x69];
    expect_eq(parsePim(oob_hello[0..]) == null, true);

    // 6. Writer rejects invalid msg_type and short buffer
    let small: [6]u8 = [0; 6];
    let w_bad = PimWriter.init(small[0..], 9);
    expect_eq(w_bad.finishIpv4() == null, true);

    let w_ok = PimWriter.init(small[0..], PIM_HELLO);
    expect_eq(w_ok.writeHoldtime(105), false); // needs 6B > 2B remaining
});
