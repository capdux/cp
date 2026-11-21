// Source: cp/cap/test/test_ipfix_production_deep.cp (Production deep tests for cap:ipfix)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    IPFIX_PORT,
    IPFIX_TLS_PORT,
    IPFIX_VERSION,
    IPFIX_HEADER_SIZE,
    IPFIX_SET_HEADER_SIZE,
    SET_ID_TEMPLATE,
    SET_ID_OPTIONS_TEMPLATE,
    SET_ID_DATA_MIN,
    IE_ENTERPRISE_BIT,
    IE_VARLEN_INDICATOR,
    IpfixHeader,
    IpfixFieldSpec,
    IpfixFieldSpecIter,
    IpfixTemplateRecord,
    IpfixTemplateIter,
    IpfixSet,
    IpfixSetIter,
    IpfixMessage,
    isIpfix,
    parseIpfix,
    IpfixVarLenValue,
    decodeIpfixVarLen,
} from "cap:ipfix";

test("1. 16B IPFIX v10 message with Template Set + Data Set zero-copy iteration", () => {
    expect_eq(IPFIX_PORT, 4739 as u16);
    expect_eq(IPFIX_TLS_PORT, 4740 as u16);
    expect_eq(IPFIX_VERSION, 10 as u16);
    expect_eq(IPFIX_HEADER_SIZE, 16);
    expect_eq(IPFIX_SET_HEADER_SIZE, 4);

    // Total message length = 16 (hdr) + 16 (Template Set) + 12 (Data Set) = 44 bytes
    const msg_bytes: [44]u8 = [
        // 16B IPFIX Header
        0x00, 0x0A,             // version = 10
        0x00, 44,               // length = 44
        0x65, 0x50, 0x00, 0x01, // export_time
        0x00, 0x00, 0x00, 0x07, // sequence_number = 7
        0x00, 0x00, 0x01, 0x00, // observation_domain_id = 256
        // Template Set (set_id=2, len=16): 1 template (id=256, field_count=2)
        0x00, 0x02, 0x00, 16,
        0x01, 0x00, 0x00, 0x02, // template_id = 256, field_count = 2
        0x00, 0x08, 0x00, 0x04, // IE 8 (sourceIPv4Address), len 4
        0x00, 0x0C, 0x00, 0x04, // IE 12 (destinationIPv4Address), len 4
        // Data Set (set_id=256, len=12): 8B data record
        0x01, 0x00, 0x00, 12,
        10, 0, 0, 1,
        10, 0, 0, 2,
    ];

    expect_eq(isIpfix(msg_bytes[0..]), true);
    const msg = parseIpfix(msg_bytes[0..]) ?? unreachable;
    expect_eq(msg.header.version, 10 as u16);
    expect_eq(msg.header.length, 44 as u16);
    expect_eq(msg.header.sequence_number, 7 as u32);
    expect_eq(msg.header.observation_domain_id, 256 as u32);

    let sit = msg.sets();
    const s0 = sit.next() ?? unreachable;
    expect_eq(s0.isTemplate(), true);
    expect_eq(s0.isData(), false);
    expect_eq(s0.length, 16 as u16);

    let tit = s0.templates();
    const tpl = tit.next() ?? unreachable;
    expect_eq(tpl.template_id, 256 as u16);
    expect_eq(tpl.field_count, 2 as u16);
    expect_eq(tit.next() == null, true);
    expect_eq(tit.hasError(), false);

    let fit = tpl.fields();
    const f0 = fit.next() ?? unreachable;
    expect_eq(f0.ie_id, 8 as u16);
    expect_eq(f0.field_length, 4 as u16);
    expect_eq(f0.isEnterprise(), false);
    const f1 = fit.next() ?? unreachable;
    expect_eq(f1.ie_id, 12 as u16);
    expect_eq(fit.next() == null, true);

    const s1 = sit.next() ?? unreachable;
    expect_eq(s1.isData(), true);
    expect_eq(s1.set_id, 256 as u16);
    expect_eq(s1.body.len, 8);
    expect_eq(s1.body[0], 10 as u8);
    expect_eq(s1.body[7], 2 as u8);
    expect_eq(sit.next() == null, true);
});

test("2. Mixed 4B standard IE and 8B Enterprise IE template with trailing zero padding", () => {
    // Template Set body:
    // Record 1: tpl_id=300, count=2 -> standard IE 1 (len=8) + enterprise IE (0x8000|42, len=0xFFFF, pen=29305)
    // Plus 2 bytes trailing zero padding
    const set_body: [18]u8 = [
        0x01, 0x2C, 0x00, 0x02, // template_id=300, field_count=2
        0x00, 0x01, 0x00, 0x08, // IE 1 (octetDeltaCount), len=8
        0x80, 0x2A, 0xFF, 0xFF, // Enterprise IE 42, variable-length (0xFFFF)
        0x00, 0x00, 0x72, 0x79, // Enterprise Number = 29305
        0x00, 0x00,             // 2B trailing zero padding
    ];

    let tit = IpfixTemplateIter.init(set_body[0..]);
    const rec = tit.next() ?? unreachable;
    expect_eq(rec.template_id, 300 as u16);
    expect_eq(rec.field_count, 2 as u16);

    let fit = rec.fields();
    const f0 = fit.next() ?? unreachable;
    expect_eq(f0.ie_id, 1 as u16);
    expect_eq(f0.field_length, 8 as u16);
    expect_eq(f0.isEnterprise(), false);
    expect_eq(f0.isVariableLength(), false);

    const f1 = fit.next() ?? unreachable;
    expect_eq(f1.ie_id, 42 as u16);
    expect_eq(f1.isEnterprise(), true);
    expect_eq(f1.isVariableLength(), true);
    expect_eq(f1.enterprise_number ?? 0, 29305 as u32);
    expect_eq(fit.next() == null, true);
    expect_eq(fit.hasError(), false);

    // Trailing 2B zero padding ends iteration cleanly
    expect_eq(tit.next() == null, true);
    expect_eq(tit.hasError(), false);
});

test("3. RFC 7011 Section 7 short (<255) and extended (255 + 2B BE) variable-length decoding", () => {
    // Short varlen: len=5, "hello"
    const short_v: [6]u8 = [5, 'h', 'e', 'l', 'l', 'o'];
    const dec_s = decodeIpfixVarLen(short_v[0..]) ?? unreachable;
    expect_eq(dec_s.consumed, 6);
    expect_eq(std.mem.eql(u8, dec_s.value, "hello"), true);

    // Zero-length varlen: len=0
    const zero_v: [1]u8 = [0];
    const dec_z = decodeIpfixVarLen(zero_v[0..]) ?? unreachable;
    expect_eq(dec_z.consumed, 1);
    expect_eq(dec_z.value.len, 0);

    // Extended varlen: 255 + u16 BE (260) + 260B payload
    let long_buf: [263]u8 = std.mem.zeroes([263]u8);
    long_buf[0] = 255;
    long_buf[1] = 0x01;
    long_buf[2] = 0x04; // 260
    long_buf[3] = 0xAA;
    long_buf[262] = 0xBB;
    const dec_l = decodeIpfixVarLen(long_buf[0..]) ?? unreachable;
    expect_eq(dec_l.consumed, 263);
    expect_eq(dec_l.value.len, 260);
    expect_eq(dec_l.value[0], 0xAA as u8);
    expect_eq(dec_l.value[259], 0xBB as u8);

    // Truncated varlen rejected
    expect_eq(decodeIpfixVarLen("") == null, true);
    const bad_s: [3]u8 = [5, 'a', 'b'];
    expect_eq(decodeIpfixVarLen(bad_s[0..]) == null, true);
    const bad_ext_hdr: [2]u8 = [255, 0x00];
    expect_eq(decodeIpfixVarLen(bad_ext_hdr[0..]) == null, true);
    const bad_ext_body: [5]u8 = [255, 0x00, 10, 1, 2];
    expect_eq(decodeIpfixVarLen(bad_ext_body[0..]) == null, true);
});

test("4. Options Template Set (Set ID 3) identification", () => {
    const msg_bytes: [24]u8 = [
        0x00, 0x0A, 0x00, 24,
        0, 0, 0, 1,
        0, 0, 0, 0,
        0, 0, 0, 1,
        // Options Template Set (set_id = 3, len = 8)
        0x00, 0x03, 0x00, 8,
        0x01, 0x01, 0x00, 0x00,
    ];
    const msg = parseIpfix(msg_bytes[0..]) ?? unreachable;
    let sit = msg.sets();
    const s0 = sit.next() ?? unreachable;
    expect_eq(s0.set_id, SET_ID_OPTIONS_TEMPLATE);
    expect_eq(s0.isOptionsTemplate(), true);
    expect_eq(s0.isTemplate(), false);
    expect_eq(s0.isData(), false);
});

test("5. Invalid version, length < 16, Set length < 4, and truncated Enterprise IE rejection", () => {
    // 1. Shorter than 16B header
    const short15: [15]u8 = [0; 15];
    expect_eq(isIpfix(short15[0..]), false);
    expect_eq(parseIpfix(short15[0..]) == null, true);

    // 2. Wrong version (NetFlow v9 instead of IPFIX v10)
    let bad_ver: [16]u8 = std.mem.zeroes([16]u8);
    bad_ver[1] = 9;
    bad_ver[3] = 16;
    expect_eq(isIpfix(bad_ver[0..]), false);
    expect_eq(parseIpfix(bad_ver[0..]) == null, true);

    // 3. Message length < 16
    let bad_len: [16]u8 = std.mem.zeroes([16]u8);
    bad_len[1] = 10;
    bad_len[3] = 12;
    expect_eq(isIpfix(bad_len[0..]), false);
    expect_eq(parseIpfix(bad_len[0..]) == null, true);

    // 4. Set with length < 4 (length = 3)
    const bad_set: [20]u8 = [
        0x00, 0x0A, 0x00, 20,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x00, 0x02, 0x00, 0x03, // set length = 3 < 4
    ];
    expect_eq(parseIpfix(bad_set[0..]) == null, true);

    // 5. Truncated Enterprise IE (sets enterprise bit 0x8000, but missing 4B enterprise number)
    const trunc_ent: [8]u8 = [
        0x01, 0x00, 0x00, 0x01, // tpl_id=256, field_count=1
        0x80, 0x01, 0x00, 0x04, // Enterprise bit set, needs 8B total but only 4B present
    ];
    let tit = IpfixTemplateIter.init(trunc_ent[0..]);
    expect_eq(tit.next() == null, true);
    expect_eq(tit.hasError(), true);
});
