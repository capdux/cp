// Source: cp/cap/test/test_bacnet_production_deep.cp (Production deep tests for cap:bacnet)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BACNET_UDP_PORT,
    BVLC_TYPE_IPV4,
    BVLC_HEADER_SIZE,
    NPDU_VERSION,
    BVLC_RESULT,
    BVLC_FORWARDED_NPDU,
    BVLC_REGISTER_FOREIGN_DEVICE,
    BVLC_ORIGINAL_UNICAST,
    BVLC_ORIGINAL_BROADCAST,
    APDU_CONFIRMED_REQ,
    APDU_UNCONFIRMED_REQ,
    APDU_SIMPLE_ACK,
    APDU_COMPLEX_ACK,
    APDU_SEGMENT_ACK,
    APDU_ERROR,
    APDU_REJECT,
    APDU_ABORT,
    SVC_I_AM,
    SVC_WHO_IS,
    SVC_READ_PROPERTY,
    SVC_READ_PROPERTY_MULTIPLE,
    SVC_WRITE_PROPERTY,
    isBvlc,
    BvlcPacket,
    parseBvlc,
    encodeBvlcUnicast,
    encodeBvlcBroadcast,
    NpduPacket,
    parseNpdu,
    ApduHeader,
    parseApdu,
    BacnetObjectId,
    decodeObjectId,
    encodeObjectId,
    BacnetTag,
    BacnetTagIter,
} from "cap:net";

test("1. Constants and BVLC Original-Unicast, Broadcast, and Forwarded-NPDU parse/encode", () => {
    expect_eq(BACNET_UDP_PORT, 47808 as u16);
    expect_eq(BVLC_TYPE_IPV4, 0x81 as u8);
    expect_eq(BVLC_HEADER_SIZE, 4 as usize);
    expect_eq(NPDU_VERSION, 0x01 as u8);
    expect_eq(BVLC_RESULT, 0x00 as u8);
    expect_eq(BVLC_FORWARDED_NPDU, 0x04 as u8);
    expect_eq(BVLC_REGISTER_FOREIGN_DEVICE, 0x05 as u8);
    expect_eq(BVLC_ORIGINAL_UNICAST, 0x0A as u8);
    expect_eq(BVLC_ORIGINAL_BROADCAST, 0x0B as u8);

    // Encode & parse Original-Unicast-NPDU
    let npdu_bytes: [4]u8 = [0x01, 0x04, 0x00, 0x05];
    let out: [32]u8 = std.mem.zeroes([32]u8);
    const n = encodeBvlcUnicast(npdu_bytes[0..], out[0..]) ?? unreachable;
    expect_eq(n, 8 as usize);
    expect_eq(isBvlc(out[0..n]), true);

    const u_pkt = parseBvlc(out[0..n]) ?? unreachable;
    expect_eq(u_pkt.bvlc_type, BVLC_TYPE_IPV4);
    expect_eq(u_pkt.function, BVLC_ORIGINAL_UNICAST);
    expect_eq(u_pkt.length, 8 as u16);
    expect_eq(u_pkt.has_forwarded, false);
    expect_eq(std.mem.eql(u8, u_pkt.payload, npdu_bytes[0..]), true);

    // Encode & parse Original-Broadcast-NPDU
    const bn = encodeBvlcBroadcast(npdu_bytes[0..], out[0..]) ?? unreachable;
    const b_pkt = parseBvlc(out[0..bn]) ?? unreachable;
    expect_eq(b_pkt.function, BVLC_ORIGINAL_BROADCAST);
    expect_eq(b_pkt.has_forwarded, false);

    // Parse Forwarded-NPDU (192.168.1.50:47808 -> 0xC0 0xA8 0x01 0x32, 0xBA 0xC0)
    let fwd: [14]u8 = [
        0x81, 0x04, 0x00, 0x0E,
        192, 168, 1, 50, 0xBA, 0xC0,
        0x01, 0x00, 0x10, 0x08,
    ];
    const f_pkt = parseBvlc(fwd[0..]) ?? unreachable;
    expect_eq(f_pkt.function, BVLC_FORWARDED_NPDU);
    expect_eq(f_pkt.has_forwarded, true);
    expect_eq(f_pkt.forwarded_ip[0], 192 as u8);
    expect_eq(f_pkt.forwarded_ip[1], 168 as u8);
    expect_eq(f_pkt.forwarded_ip[2], 1 as u8);
    expect_eq(f_pkt.forwarded_ip[3], 50 as u8);
    expect_eq(f_pkt.forwarded_port, 47808 as u16);
    expect_eq(f_pkt.payload.len, 4 as usize);
});

test("2. NPDU parsing with and without DNET/DADR, SNET/SADR, and HopCount", () => {
    // Minimal NPDU: version=1, control=0x04 (expecting reply, normal priority 0) + 2B APDU
    let simple: [4]u8 = [0x01, 0x04, 0x10, 0x08];
    const n1 = parseNpdu(simple[0..]) ?? unreachable;
    expect_eq(n1.version, 0x01 as u8);
    expect_eq(n1.isNetworkMessage(), false);
    expect_eq(n1.expectingReply(), true);
    expect_eq(n1.priority(), 0 as u8);
    expect_eq(n1.dnet == null, true);
    expect_eq(n1.snet == null, true);
    expect_eq(n1.hop_count == null, true);
    expect_eq(n1.apdu.len, 2 as usize);

    // Full routed NPDU:
    // control = 0x20 (DNET) | 0x08 (SNET) | 0x04 (reply) | 0x02 (urgent priority) = 0x2E
    // DNET=0x000A, DLEN=1, DADR=[0x15]
    // SNET=0x0014, SLEN=2, SADR=[0xAA, 0xBB]
    // HopCount=0xFE, APDU=[0x10, 0x00]
    let routed: [14]u8 = [
        0x01, 0x2E,
        0x00, 0x0A, 0x01, 0x15,
        0x00, 0x14, 0x02, 0xAA, 0xBB,
        0xFE,
        0x10, 0x00,
    ];
    const n2 = parseNpdu(routed[0..]) ?? unreachable;
    expect_eq(n2.isNetworkMessage(), false);
    expect_eq(n2.expectingReply(), true);
    expect_eq(n2.priority(), 2 as u8);
    expect_eq(n2.dnet ?? 0, 10 as u16);
    expect_eq(n2.dadr.len, 1 as usize);
    expect_eq(n2.dadr[0], 0x15 as u8);
    expect_eq(n2.snet ?? 0, 20 as u16);
    expect_eq(n2.sadr.len, 2 as usize);
    expect_eq(n2.sadr[0], 0xAA as u8);
    expect_eq(n2.sadr[1], 0xBB as u8);
    expect_eq(n2.hop_count ?? 0, 0xFE as u8);
    expect_eq(n2.apdu.len, 2 as usize);
});

test("3. APDU parsing (Confirmed ReadProperty, Unconfirmed I-Am, Complex-ACK, Error) and Object ID codec", () => {
    // Confirmed-Request ReadProperty: [0x00, 0x05(max-apdu), 0x2A(invoke=42), 0x0C(ReadProperty), payload...]
    let read_prop: [10]u8 = [0x00, 0x05, 0x2A, SVC_READ_PROPERTY, 0x0C, 0x02, 0x00, 0x00, 0x01, 0x19];
    const a1 = parseApdu(read_prop[0..]) ?? unreachable;
    expect_eq(a1.pdu_type, APDU_CONFIRMED_REQ);
    expect_eq(a1.invoke_id ?? 0, 42 as u8);
    expect_eq(a1.service_choice, SVC_READ_PROPERTY);
    expect_eq(a1.payload.len, 6 as usize);

    // Unconfirmed-Request I-Am: [0x10, 0x00(I-Am), payload...]
    let i_am: [7]u8 = [0x10, SVC_I_AM, 0xC4, 0x02, 0x00, 0x03, 0xE8];
    const a2 = parseApdu(i_am[0..]) ?? unreachable;
    expect_eq(a2.pdu_type, APDU_UNCONFIRMED_REQ);
    expect_eq(a2.invoke_id == null, true);
    expect_eq(a2.service_choice, SVC_I_AM);
    expect_eq(a2.payload.len, 5 as usize);

    // Complex-ACK & Error
    let cack: [5]u8 = [0x30, 0x07, SVC_READ_PROPERTY, 0x01, 0x02];
    const a3 = parseApdu(cack[0..]) ?? unreachable;
    expect_eq(a3.pdu_type, APDU_COMPLEX_ACK);
    expect_eq(a3.invoke_id ?? 0, 7 as u8);
    expect_eq(a3.service_choice, SVC_READ_PROPERTY);
    expect_eq(a3.payload.len, 2 as usize);

    let err_apdu: [4]u8 = [0x50, 0x07, SVC_WRITE_PROPERTY, 0x91];
    const a4 = parseApdu(err_apdu[0..]) ?? unreachable;
    expect_eq(a4.pdu_type, APDU_ERROR);
    expect_eq(a4.invoke_id ?? 0, 7 as u8);
    expect_eq(a4.service_choice, SVC_WRITE_PROPERTY);

    // 32-bit Object ID roundtrip: Device (type=8), instance=1000 -> (8 << 22) | 1000 = 0x020003E8
    const raw_id = encodeObjectId(8, 1000) ?? unreachable;
    expect_eq(raw_id, 0x020003E8 as u32);
    const decoded = decodeObjectId(raw_id);
    expect_eq(decoded.object_type, 8 as u16);
    expect_eq(decoded.instance, 1000 as u32);

    // Boundary check: max 10-bit type (1023) & max 22-bit instance (4194303)
    const max_id = encodeObjectId(1023, 4194303) ?? unreachable;
    expect_eq(max_id, 0xFFFFFFFF as u32);
    expect_eq(encodeObjectId(1024, 0) == null, true);
    expect_eq(encodeObjectId(0, 4194304) == null, true);
});

test("4. BacnetTagIter: standard tags, Context Opening/Closing, extended tag_number==15, and extended length", () => {
    // Sequence:
    // 1) Context tag 0, len 4 (Object ID): 0x0C, 0x02, 0x00, 0x03, 0xE8
    // 2) Context Opening tag 3: 0x3E
    // 3) Application CharacterString (tag 7), ext length lvt=5 -> ext=6: 0x75, 0x06, 0x00, 'H', 'V', 'A', 'C', '1'
    // 4) Extended context tag_number=15 -> real tag 20, len 2: 0xFA, 0x14, 0x12, 0x34
    // 5) Context Closing tag 3: 0x3F
    let tlv: [19]u8 = [
        0x0C, 0x02, 0x00, 0x03, 0xE8,
        0x3E,
        0x75, 0x06, 0x00, 'H', 'V', 'A', 'C', '1',
        0xFA, 0x14, 0x12, 0x34,
        0x3F,
    ];

    let it = BacnetTagIter.init(tlv[0..]);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.tag_number, 0 as u8);
    expect_eq(t1.is_context, true);
    expect_eq(t1.is_opening, false);
    expect_eq(t1.is_closing, false);
    expect_eq(t1.value.len, 4 as usize);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.tag_number, 3 as u8);
    expect_eq(t2.is_context, true);
    expect_eq(t2.is_opening, true);
    expect_eq(t2.is_closing, false);
    expect_eq(t2.value.len, 0 as usize);

    const t3 = it.next() ?? unreachable;
    expect_eq(t3.tag_number, 7 as u8);
    expect_eq(t3.is_context, false);
    expect_eq(t3.value.len, 6 as usize);
    expect_eq(std.mem.eql(u8, t3.value[1..], "HVAC1"), true);

    const t4 = it.next() ?? unreachable;
    expect_eq(t4.tag_number, 20 as u8);
    expect_eq(t4.is_context, true);
    expect_eq(t4.value.len, 2 as usize);
    expect_eq(t4.value[0], 0x12 as u8);
    expect_eq(t4.value[1], 0x34 as u8);

    const t5 = it.next() ?? unreachable;
    expect_eq(t5.tag_number, 3 as u8);
    expect_eq(t5.is_context, true);
    expect_eq(t5.is_opening, false);
    expect_eq(t5.is_closing, true);

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
});

test("5. Malformed packets: invalid BVLC type, bad NPDU version, truncated headers, and out-of-bounds TLV rejection", () => {
    // Bad BVLC type (0x82 instead of 0x81)
    let bad_bvlc: [4]u8 = [0x82, 0x0A, 0x00, 0x04];
    expect_eq(isBvlc(bad_bvlc[0..]), false);
    expect_eq(parseBvlc(bad_bvlc[0..]) == null, true);

    // BVLC length larger than buffer or < 4
    let over_bvlc: [4]u8 = [0x81, 0x0A, 0x00, 0x20];
    expect_eq(parseBvlc(over_bvlc[0..]) == null, true);
    let short_bvlc: [4]u8 = [0x81, 0x0A, 0x00, 0x03];
    expect_eq(parseBvlc(short_bvlc[0..]) == null, true);

    // Truncated Forwarded-NPDU (length < 10)
    let trunc_fwd: [8]u8 = [0x81, 0x04, 0x00, 0x08, 192, 168, 1, 1];
    expect_eq(parseBvlc(trunc_fwd[0..]) == null, true);

    // Bad NPDU version (0x02) & truncated DADR/HopCount
    let bad_npdu: [2]u8 = [0x02, 0x00];
    expect_eq(parseNpdu(bad_npdu[0..]) == null, true);
    let trunc_npdu: [5]u8 = [0x01, 0x20, 0x00, 0x01, 0x04]; // claims DLEN=4 but 0 bytes left
    expect_eq(parseNpdu(trunc_npdu[0..]) == null, true);

    // Invalid APDU type (>= 8) & truncated Confirmed-Request
    let bad_apdu: [4]u8 = [0x80, 0x00, 0x01, 0x02];
    expect_eq(parseApdu(bad_apdu[0..]) == null, true);
    let trunc_creq: [3]u8 = [0x00, 0x05, 0x01];
    expect_eq(parseApdu(trunc_creq[0..]) == null, true);

    // Truncated TLV value sets hasError()
    let trunc_tlv: [3]u8 = [0x0C, 0x01, 0x02]; // claims 4 bytes, only 2 present
    let bad_it = BacnetTagIter.init(trunc_tlv[0..]);
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.hasError(), true);

    // Invalid Application tag with lvt == 6 sets hasError()
    let bad_app_tag: [1]u8 = [0x26];
    let bad_it2 = BacnetTagIter.init(bad_app_tag[0..]);
    expect_eq(bad_it2.next() == null, true);
    expect_eq(bad_it2.hasError(), true);
});
