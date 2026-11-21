// Source: cp/cap/test/test_pppoe_production_deep.cp (Production deep tests for cap:pppoe)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PPPOE_ETHERTYPE_DISCOVERY,
    PPPOE_ETHERTYPE_SESSION,
    PPPOE_VERSION,
    PPPOE_TYPE,
    PPPOE_HEADER_SIZE,
    PPPOE_TAG_HEADER_SIZE,
    CODE_SESSION,
    CODE_PADO,
    CODE_PADI,
    CODE_PADR,
    CODE_PADS,
    CODE_PADT,
    TAG_END_OF_LIST,
    TAG_SERVICE_NAME,
    TAG_AC_NAME,
    TAG_HOST_UNIQ,
    TAG_AC_COOKIE,
    TAG_VENDOR_SPECIFIC,
    TAG_RELAY_SESSION_ID,
    TAG_SERVICE_NAME_ERROR,
    TAG_AC_SYSTEM_ERROR,
    TAG_GENERIC_ERROR,
    PppoeHeader,
    PppoeTag,
    PppoeTagIter,
    PppoePacket,
    isPppoe,
    parsePppoe,
    PppoeWriter,
} from "cap:pppoe";

test("1. PADI / PADO / PADR / PADS / PADT Discovery TLV roundtrip", () => {
    expect_eq(PPPOE_ETHERTYPE_DISCOVERY, 0x8863 as u16);
    expect_eq(PPPOE_ETHERTYPE_SESSION, 0x8864 as u16);
    expect_eq(PPPOE_HEADER_SIZE, 6);
    expect_eq(PPPOE_TAG_HEADER_SIZE, 4);

    // PADO with Service-Name, AC-Name, Host-Uniq, AC-Cookie
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let w = PppoeWriter.init(buf[0..], CODE_PADO, 0);
    expect_eq(w.writeTag(TAG_SERVICE_NAME, "broadband"), true);
    expect_eq(w.writeTag(TAG_AC_NAME, "bras-syd-01"), true);
    expect_eq(w.writeTag(TAG_HOST_UNIQ, "H0ST"), true);
    expect_eq(w.writeTag(TAG_AC_COOKIE, "C00KIE99"), true);
    const wire = w.finish() ?? unreachable;
    expect_eq(isPppoe(wire), true);

    const pkt = parsePppoe(wire) ?? unreachable;
    expect_eq(pkt.header.version, PPPOE_VERSION);
    expect_eq(pkt.header.pkt_type, PPPOE_TYPE);
    expect_eq(pkt.header.code, CODE_PADO);
    expect_eq(pkt.header.session_id, 0 as u16);
    expect_eq(pkt.isDiscovery(), true);
    expect_eq(pkt.isSession(), false);
    expect_eq(std.mem.eql(u8, pkt.serviceName() ?? unreachable, "broadband"), true);
    expect_eq(std.mem.eql(u8, pkt.acName() ?? unreachable, "bras-syd-01"), true);
    expect_eq(std.mem.eql(u8, pkt.hostUniq() ?? unreachable, "H0ST"), true);
    expect_eq(std.mem.eql(u8, pkt.acCookie() ?? unreachable, "C00KIE99"), true);

    // PADI, PADR, PADS, PADT codes
    const codes: [4]u8 = [CODE_PADI, CODE_PADR, CODE_PADS, CODE_PADT];
    let i: usize = 0;
    while (i < 4) {
        let b: [32]u8 = std.mem.zeroes([32]u8);
        let wd = PppoeWriter.init(b[0..], codes[i], if (codes[i] == CODE_PADS || codes[i] == CODE_PADT) 0x1234 else 0);
        expect_eq(wd.writeTag(TAG_SERVICE_NAME, ""), true);
        const w_out = wd.finish() ?? unreachable;
        const p = parsePppoe(w_out) ?? unreachable;
        expect_eq(p.header.code, codes[i]);
        expect_eq(p.isDiscovery(), true);
        i += 1;
    }
});

test("2. PPPoE Session frame with PPP payload and trailing Ethernet padding", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let w = PppoeWriter.init(buf[0..], CODE_SESSION, 0x0042);
    // PPP IPv4 protocol 0x0021 + stub bytes
    const ppp_frame: [6]u8 = [0x00, 0x21, 0x45, 0x00, 0x00, 0x14];
    expect_eq(w.writePayload(ppp_frame[0..]), true);
    const wire = w.finish() ?? unreachable;
    expect_eq(wire.len, 12);

    // Simulate Ethernet frame padded to 46 bytes (extra zero bytes after PPPoE length)
    const pkt = parsePppoe(buf[0..46]) ?? unreachable;
    expect_eq(pkt.isSession(), true);
    expect_eq(pkt.isDiscovery(), false);
    expect_eq(pkt.header.session_id, 0x0042 as u16);
    expect_eq(pkt.header.length, 6 as u16);
    expect_eq(pkt.payload.len, 6);
    expect_eq(std.mem.eql(u8, pkt.payload, ppp_frame[0..]), true);
});

test("3. TAG_END_OF_LIST stops Discovery tag iteration cleanly", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PppoeWriter.init(buf[0..], CODE_PADI, 0);
    expect_eq(w.writeTag(TAG_SERVICE_NAME, "svc-a"), true);
    expect_eq(w.writeEndOfList(), true);
    // Tag placed after END_OF_LIST must be ignored by iterator
    expect_eq(w.writeTag(TAG_AC_NAME, "should-not-see"), true);
    const wire = w.finish() ?? unreachable;

    const pkt = parsePppoe(wire) ?? unreachable;
    expect_eq(std.mem.eql(u8, pkt.serviceName() ?? unreachable, "svc-a"), true);
    expect_eq(pkt.acName() == null, true);

    let it = pkt.tags();
    const t0 = it.next() ?? unreachable;
    expect_eq(t0.tag_type, TAG_SERVICE_NAME);
    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("4. Error and vendor/relay Discovery tag lookup", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = PppoeWriter.init(buf[0..], CODE_PADS, 0);
    expect_eq(w.writeTag(TAG_SERVICE_NAME_ERROR, "unknown-svc"), true);
    expect_eq(w.writeTag(TAG_AC_SYSTEM_ERROR, "busy"), true);
    expect_eq(w.writeTag(TAG_GENERIC_ERROR, "err"), true);
    expect_eq(w.writeTag(TAG_VENDOR_SPECIFIC, "VEND"), true);
    expect_eq(w.writeTag(TAG_RELAY_SESSION_ID, "REL1"), true);
    const wire = w.finish() ?? unreachable;

    const pkt = parsePppoe(wire) ?? unreachable;
    expect_eq(std.mem.eql(u8, (pkt.findTag(TAG_SERVICE_NAME_ERROR) ?? unreachable).value, "unknown-svc"), true);
    expect_eq(std.mem.eql(u8, (pkt.findTag(TAG_AC_SYSTEM_ERROR) ?? unreachable).value, "busy"), true);
    expect_eq(std.mem.eql(u8, (pkt.findTag(TAG_GENERIC_ERROR) ?? unreachable).value, "err"), true);
    expect_eq(std.mem.eql(u8, (pkt.findTag(TAG_VENDOR_SPECIFIC) ?? unreachable).value, "VEND"), true);
    expect_eq(std.mem.eql(u8, (pkt.findTag(TAG_RELAY_SESSION_ID) ?? unreachable).value, "REL1"), true);
});

test("5. Invalid version/type, session_id=0, unknown code, truncated and out-of-bounds tag rejection", () => {
    // 1. Shorter than 6B header
    const short5: [5]u8 = [0x11, CODE_PADI, 0, 0, 0];
    expect_eq(isPppoe(short5[0..]), false);
    expect_eq(parsePppoe(short5[0..]) == null, true);

    // 2. Invalid version (0x21) and invalid type (0x12)
    const bad_ver: [6]u8 = [0x21, CODE_PADI, 0, 0, 0, 0];
    const bad_type: [6]u8 = [0x12, CODE_PADI, 0, 0, 0, 0];
    expect_eq(isPppoe(bad_ver[0..]), false);
    expect_eq(parsePppoe(bad_ver[0..]) == null, true);
    expect_eq(isPppoe(bad_type[0..]), false);
    expect_eq(parsePppoe(bad_type[0..]) == null, true);

    // 3. Session packet with session_id == 0 rejected
    const bad_sess: [6]u8 = [0x11, CODE_SESSION, 0x00, 0x00, 0x00, 0x00];
    expect_eq(parsePppoe(bad_sess[0..]) == null, true);

    // 4. Unknown PPPoE code (0x55) rejected
    const bad_code: [6]u8 = [0x11, 0x55, 0x00, 0x01, 0x00, 0x00];
    expect_eq(parsePppoe(bad_code[0..]) == null, true);

    // 5. Length exceeds available bytes (length = 10, only 6B total)
    const oob_len: [6]u8 = [0x11, CODE_PADI, 0x00, 0x00, 0x00, 10];
    expect_eq(isPppoe(oob_len[0..]), false);
    expect_eq(parsePppoe(oob_len[0..]) == null, true);

    // 6. Discovery tag length exceeds payload bounds (payload=4B, tag_len=8B)
    const oob_tag: [10]u8 = [0x11, CODE_PADI, 0x00, 0x00, 0x00, 4, 0x01, 0x01, 0x00, 8];
    expect_eq(parsePppoe(oob_tag[0..]) == null, true);

    // 7. Writer rejects session_id=0 for SESSION and buffer overflow
    let small: [8]u8 = [0; 8];
    let w_bad_s = PppoeWriter.init(small[0..], CODE_SESSION, 0);
    expect_eq(w_bad_s.finish() == null, true);

    let w_ok = PppoeWriter.init(small[0..], CODE_PADI, 0);
    expect_eq(w_ok.writeTag(TAG_SERVICE_NAME, "abc"), false); // needs 7B > 2B remaining
});
