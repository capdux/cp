// Source: cp/cap/test/test_radius_production_deep.cp (Production deep tests for cap:radius)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RADIUS_HEADER_SIZE,
    RADIUS_MAX_PACKET_LEN,
    RADIUS_AUTH_PORT,
    RADIUS_ACCT_PORT,
    ATTR_USER_NAME,
    ATTR_FRAMED_IP_ADDRESS,
    ATTR_REPLY_MESSAGE,
    ATTR_VENDOR_SPECIFIC,
    ATTR_SESSION_TIMEOUT,
    ATTR_NAS_IDENTIFIER,
    ATTR_ACCT_STATUS_TYPE,
    ATTR_ACCT_SESSION_ID,
    ATTR_MESSAGE_AUTHENTICATOR,
    RadiusCode,
    RadiusAttr,
    RadiusVsa,
    RadiusPacket,
    RadiusAttrIter,
    RadiusVsaIter,
    RadiusWriter,
    parseRadiusPacket,
} from "cap:radius";

test("1. Access-Request, Access-Accept, and Accounting-Request full codec roundtrip", () => {
    expect_eq(RADIUS_HEADER_SIZE, 20);
    expect_eq(RADIUS_MAX_PACKET_LEN, 4096);
    expect_eq(RADIUS_AUTH_PORT, 1812);
    expect_eq(RADIUS_ACCT_PORT, 1813);

    let buf: [256]u8 = [0; 256];
    let req_auth: [16]u8 = [0xAA; 16];
    let w = RadiusWriter.init(buf[0..], RadiusCode.access_request, 42, req_auth);
    expect_eq(w.writeAttr(ATTR_USER_NAME, "alice@corp.io"), true);
    expect_eq(w.writeAttr(ATTR_NAS_IDENTIFIER, "edge-gw-01"), true);

    const encoded = w.finish() ?? unreachable;
    const pkt = parseRadiusPacket(encoded) ?? unreachable;
    expect_eq(pkt.code == RadiusCode.access_request, true);
    expect_eq(pkt.raw_code, 1);
    expect_eq(pkt.identifier, 42);
    expect_eq(usize(pkt.length), encoded.len);
    expect_eq(pkt.authenticator[0], 0xAA);
    expect_eq(pkt.authenticator[15], 0xAA);

    // Accounting-Request roundtrip
    let acct_buf: [128]u8 = [0; 128];
    let acct_auth: [16]u8 = [0x55; 16];
    let wa = RadiusWriter.init(acct_buf[0..], RadiusCode.accounting_request, 9, acct_auth);
    expect_eq(wa.writeU32Attr(ATTR_ACCT_STATUS_TYPE, 1), true); // Start
    expect_eq(wa.writeAttr(ATTR_ACCT_SESSION_ID, "sess-9981"), true);
    const acct_raw = wa.finish() ?? unreachable;
    const acct_pkt = parseRadiusPacket(acct_raw) ?? unreachable;
    expect_eq(acct_pkt.code == RadiusCode.accounting_request, true);
    expect_eq(acct_pkt.findU32Attr(ATTR_ACCT_STATUS_TYPE) ?? 0, 1);
    expect_eq(std.mem.eql(u8, acct_pkt.findAttr(ATTR_ACCT_SESSION_ID) ?? "", "sess-9981"), true);
});

test("2. findAttr & findU32Attr decode User-Name, Framed-IP-Address, Session-Timeout & Message-Authenticator", () => {
    let buf: [256]u8 = [0; 256];
    let auth: [16]u8 = [0x12; 16];
    let msg_auth: [16]u8 = [0xEF; 16];

    let w = RadiusWriter.init(buf[0..], RadiusCode.access_accept, 100, auth);
    expect_eq(w.writeAttr(ATTR_USER_NAME, "bob"), true);
    // Framed-IP-Address = 10.20.30.40 -> 0x0A141E28
    expect_eq(w.writeU32Attr(ATTR_FRAMED_IP_ADDRESS, 0x0A141E28), true);
    expect_eq(w.writeU32Attr(ATTR_SESSION_TIMEOUT, 3600), true);
    expect_eq(w.writeAttr(ATTR_REPLY_MESSAGE, "Welcome to CP VPN"), true);
    expect_eq(w.writeAttr(ATTR_MESSAGE_AUTHENTICATOR, msg_auth[0..16]), true);

    const raw = w.finish() ?? unreachable;
    const pkt = parseRadiusPacket(raw) ?? unreachable;

    expect_eq(pkt.code == RadiusCode.access_accept, true);
    expect_eq(std.mem.eql(u8, pkt.findAttr(ATTR_USER_NAME) ?? "", "bob"), true);
    expect_eq(pkt.findU32Attr(ATTR_FRAMED_IP_ADDRESS) ?? 0, 0x0A141E28);
    expect_eq(pkt.findU32Attr(ATTR_SESSION_TIMEOUT) ?? 0, 3600);
    expect_eq(std.mem.eql(u8, pkt.findAttr(ATTR_REPLY_MESSAGE) ?? "", "Welcome to CP VPN"), true);
    const ma = pkt.findAttr(ATTR_MESSAGE_AUTHENTICATOR) ?? unreachable;
    expect_eq(ma.len, 16);
    expect_eq(ma[0], 0xEF);
    expect_eq(pkt.findAttr(200) == null, true);
});

test("3. RadiusVsaIter decodes Cisco VSA via RadiusWriter and multi-sub-TLV Microsoft VSA", () => {
    // 1) Cisco (Enterprise ID = 9) single VSA via RadiusWriter
    let buf: [128]u8 = [0; 128];
    let auth: [16]u8 = [0; 16];
    let w = RadiusWriter.init(buf[0..], RadiusCode.access_accept, 5, auth);
    expect_eq(w.writeVsa(9, 1, "shell:priv-lvl=15"), true);
    const raw = w.finish() ?? unreachable;
    const pkt = parseRadiusPacket(raw) ?? unreachable;

    const cisco_vsa_val = pkt.findAttr(ATTR_VENDOR_SPECIFIC) ?? unreachable;
    let vit = RadiusVsaIter.init(cisco_vsa_val);
    const v0 = vit.next() ?? unreachable;
    expect_eq(v0.vendor_id, 9);
    expect_eq(v0.vendor_type, 1);
    expect_eq(std.mem.eql(u8, v0.value, "shell:priv-lvl=15"), true);
    expect_eq(vit.next() == null, true);
    expect_eq(vit.malformed, false);

    // 2) Microsoft (Enterprise ID = 311 = 0x00000137) packed multi-sub-TLV VSA
    const ms_vsa_payload: []const u8 =
        "\x00\x00\x01\x37" ++     // vendor_id = 311
        "\x01\x06\xDE\xAD\xBE\xEF" ++ // sub-TLV 1: type=1, len=6, val=DE AD BE EF
        "\x1A\x05\x41\x42\x43";       // sub-TLV 2: type=26, len=5, val="ABC"
    let mvit = RadiusVsaIter.init(ms_vsa_payload);
    const mv1 = mvit.next() ?? unreachable;
    expect_eq(mv1.vendor_id, 311);
    expect_eq(mv1.vendor_type, 1);
    expect_eq(mv1.value.len, 4);
    expect_eq(mv1.value[0], 0xDE);

    const mv2 = mvit.next() ?? unreachable;
    expect_eq(mv2.vendor_id, 311);
    expect_eq(mv2.vendor_type, 26);
    expect_eq(std.mem.eql(u8, mv2.value, "ABC"), true);
    expect_eq(mvit.next() == null, true);
    expect_eq(mvit.malformed, false);
});

test("4. RadiusWriter overflow protection for tiny buffers and oversized attributes", () => {
    let auth: [16]u8 = [0; 16];

    // 缓冲区 < 20B
    let tiny: [10]u8 = [0; 10];
    let w_tiny = RadiusWriter.init(tiny[0..], RadiusCode.access_request, 1, auth);
    expect_eq(w_tiny.overflow, true);
    expect_eq(w_tiny.finish() == null, true);

    // 24B 缓冲区只能容纳 20B 报头 + 4B 属性，再写 6B 属性触发溢出
    let small: [24]u8 = [0; 24];
    let w_small = RadiusWriter.init(small[0..], RadiusCode.access_request, 1, auth);
    expect_eq(w_small.writeAttr(ATTR_USER_NAME, "ab"), true); // 2 + 2 = 4B -> pos=24
    expect_eq(w_small.writeAttr(ATTR_USER_NAME, "cd"), false);
    expect_eq(w_small.overflow, true);
    expect_eq(w_small.finish() == null, true);
});

test("5. Resilience against invalid packet length (<20, >4096, >data.len), AVP len < 2, and truncation", () => {
    // 短于 20B
    expect_eq(parseRadiusPacket("\x01\x01\x00\x14") == null, true);

    // 报头声明 length = 19 (< 20)
    let bad_len_lo: [20]u8 = [0; 20];
    bad_len_lo[0] = 1;
    bad_len_lo[3] = 19;
    expect_eq(parseRadiusPacket(bad_len_lo[0..]) == null, true);

    // 报头声明 length = 4097 (> 4096)
    let bad_len_hi: [20]u8 = [0; 20];
    bad_len_hi[0] = 1;
    bad_len_hi[2] = 0x10;
    bad_len_hi[3] = 0x01;
    expect_eq(parseRadiusPacket(bad_len_hi[0..]) == null, true);

    // 报头声明 length = 30 > data.len (20)
    let bad_len_trunc: [20]u8 = [0; 20];
    bad_len_trunc[0] = 1;
    bad_len_trunc[3] = 30;
    expect_eq(parseRadiusPacket(bad_len_trunc[0..]) == null, true);

    // AVP len < 2 (len = 1) 与截断 AVP 触发 malformed
    let ait_bad = RadiusAttrIter.init("\x01\x01\x41");
    expect_eq(ait_bad.next() == null, true);
    expect_eq(ait_bad.malformed, true);

    let ait_trunc = RadiusAttrIter.init("\x01\x08\x41\x42");
    expect_eq(ait_trunc.next() == null, true);
    expect_eq(ait_trunc.malformed, true);

    // 截断 VSA (< 4B enterprise id) 与子 TLV len < 2
    let vit_short = RadiusVsaIter.init("\x00\x00\x09");
    expect_eq(vit_short.malformed, true);
    expect_eq(vit_short.next() == null, true);

    let vit_bad_sub = RadiusVsaIter.init("\x00\x00\x00\x09\x01\x01");
    expect_eq(vit_bad_sub.next() == null, true);
    expect_eq(vit_bad_sub.malformed, true);
});
