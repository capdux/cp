// Source: cp/cap/test/test_ebml_production_deep.cp (Production deep tests for cap:ebml)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EBML_ID_HEADER,
    EBML_ID_VERSION,
    EBML_ID_DOCTYPE,
    EBML_ID_SEGMENT,
    EbmlVint,
    EbmlElement,
    decodeEbmlId,
    decodeEbmlSize,
    isEbmlHeader,
    EbmlElementIter,
    decodeEbmlUint,
} from "cap:media";

test("1. decodeEbmlId decodes 1-4 byte IDs and rejects reserved/out-of-range IDs per RFC 8794 section 5", () => {
    const id1: [1]u8 = [0xD7];
    const r1 = decodeEbmlId(id1[0..]) ?? unreachable;
    expect_eq(r1.bytes_read, 1);
    expect_eq(r1.raw_id, 0xD7);
    expect_eq(r1.value, 0x57);

    const id2: [2]u8 = [0x42, 0x82];
    const r2 = decodeEbmlId(id2[0..]) ?? unreachable;
    expect_eq(r2.bytes_read, 2);
    expect_eq(r2.raw_id, EBML_ID_DOCTYPE);
    expect_eq(r2.value, 0x0282);

    const id4: [4]u8 = [0x1A, 0x45, 0xDF, 0xA3];
    const r4 = decodeEbmlId(id4[0..]) ?? unreachable;
    expect_eq(r4.bytes_read, 4);
    expect_eq(r4.raw_id, EBML_ID_HEADER);
    expect_eq(r4.value, 0x0A45DFA3);

    // Reject empty, zero leading byte, 5-byte ID, all-zero data, all-ones data, truncated
    expect_eq(decodeEbmlId(id1[0..0]) == null, true);

    const zero_lead: [1]u8 = [0x00];
    expect_eq(decodeEbmlId(zero_lead[0..]) == null, true);

    const five_byte: [5]u8 = [0x08, 0x01, 0x02, 0x03, 0x04];
    expect_eq(decodeEbmlId(five_byte[0..]) == null, true);

    const all_zero1: [1]u8 = [0x80];
    expect_eq(decodeEbmlId(all_zero1[0..]) == null, true);

    const all_zero2: [2]u8 = [0x40, 0x00];
    expect_eq(decodeEbmlId(all_zero2[0..]) == null, true);

    const all_ones1: [1]u8 = [0xFF];
    expect_eq(decodeEbmlId(all_ones1[0..]) == null, true);

    const all_ones2: [2]u8 = [0x7F, 0xFF];
    expect_eq(decodeEbmlId(all_ones2[0..]) == null, true);

    expect_eq(decodeEbmlId(id4[0..3]) == null, true);
});

test("2. decodeEbmlSize decodes 1-8 byte VINT sizes and identifies unknown all-ones sizes", () => {
    const s1: [1]u8 = [0x85]; // size = 5
    const r1 = decodeEbmlSize(s1[0..]) ?? unreachable;
    expect_eq(r1.bytes_read, 1);
    expect_eq(r1.value, 5);
    expect_eq(r1.is_unknown_size, false);

    const s2: [2]u8 = [0x40, 0x7F]; // size = 127 in 2-byte VINT
    const r2 = decodeEbmlSize(s2[0..]) ?? unreachable;
    expect_eq(r2.bytes_read, 2);
    expect_eq(r2.value, 127);
    expect_eq(r2.is_unknown_size, false);

    // 1-byte unknown size: 0xFF
    const unk1: [1]u8 = [0xFF];
    const ru1 = decodeEbmlSize(unk1[0..]) ?? unreachable;
    expect_eq(ru1.bytes_read, 1);
    expect_eq(ru1.is_unknown_size, true);

    // 8-byte unknown size: 0x01 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
    const unk8: [8]u8 = [0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    const ru8 = decodeEbmlSize(unk8[0..]) ?? unreachable;
    expect_eq(ru8.bytes_read, 8);
    expect_eq(ru8.is_unknown_size, true);

    // 8-byte concrete size: 0x01 0x00 0x00 0x00 0x00 0x01 0x00 0x00 = 65536
    const big8: [8]u8 = [0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00];
    const rb8 = decodeEbmlSize(big8[0..]) ?? unreachable;
    expect_eq(rb8.bytes_read, 8);
    expect_eq(rb8.value, 65536);
    expect_eq(rb8.is_unknown_size, false);

    // Reject zero first byte and truncated 8-byte VINT
    const bad0: [1]u8 = [0x00];
    expect_eq(decodeEbmlSize(bad0[0..]) == null, true);
    expect_eq(decodeEbmlSize(unk8[0..7]) == null, true);
});

test("3. isEbmlHeader and recursive EbmlElementIter parse nested WebM EBML header", () => {
    // EBML Header (0x1A45DFA3), size=11 (0x8B):
    //   EBMLVersion (0x4286), size=1 (0x81), value=1 (0x01) -> 4 bytes
    //   DocType (0x4282), size=4 (0x84), value="webm" -> 7 bytes
    const webm_hdr: [16]u8 = [
        0x1A, 0x45, 0xDF, 0xA3, 0x8B,
        0x42, 0x86, 0x81, 0x01,
        0x42, 0x82, 0x84, 'w', 'e', 'b', 'm',
    ];

    expect_eq(isEbmlHeader(webm_hdr[0..]), true);
    expect_eq(isEbmlHeader(webm_hdr[0..3]), false);

    let top = EbmlElementIter.init(webm_hdr[0..]);
    const hdr = top.next() ?? unreachable;
    expect_eq(hdr.id, EBML_ID_HEADER);
    expect_eq(hdr.size, 11);
    expect_eq(hdr.is_unknown_size, false);
    expect_eq(hdr.payload.len, 11);
    expect_eq(top.next() == null, true);

    // Recursively iterate Master Element payload
    let inner = EbmlElementIter.init(hdr.payload);

    const ver_el = inner.next() ?? unreachable;
    expect_eq(ver_el.id, EBML_ID_VERSION);
    expect_eq(decodeEbmlUint(ver_el.payload) ?? 0, 1);

    const doc_el = inner.next() ?? unreachable;
    expect_eq(doc_el.id, EBML_ID_DOCTYPE);
    expect_eq(std.mem.eql(u8, doc_el.payload, "webm"), true);

    expect_eq(inner.next() == null, true);
});

test("4. Streaming unknown-size Segment element exposes remaining payload for recursive parsing", () => {
    // Segment (0x18538067), unknown 8-byte size (0x01 0xFF*7), followed by child element (0xD7, size=1, val=42)
    const stream: [15]u8 = [
        0x18, 0x53, 0x80, 0x67,
        0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xD7, 0x81, 42,
    ];

    let it = EbmlElementIter.init(stream[0..]);
    const seg = it.next() ?? unreachable;
    expect_eq(seg.id, EBML_ID_SEGMENT);
    expect_eq(seg.is_unknown_size, true);
    expect_eq(seg.payload.len, 3);
    expect_eq(it.next() == null, true);

    let sub = EbmlElementIter.init(seg.payload);
    const track = sub.next() ?? unreachable;
    expect_eq(track.id, 0xD7);
    expect_eq(decodeEbmlUint(track.payload) ?? 0, 42);
    expect_eq(sub.next() == null, true);
});

test("5. decodeEbmlUint edge cases and truncated element bounds protection", () => {
    const u0: [1]u8 = [0];
    expect_eq(decodeEbmlUint(u0[0..0]) ?? 99, 0);

    const u4: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    expect_eq(decodeEbmlUint(u4[0..]) ?? 0, 0x01020304);

    const u8: [8]u8 = [0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05];
    expect_eq(decodeEbmlUint(u8[0..]) ?? 0, ((1 as u64) << 56) | (5 as u64));

    const u9: [9]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9];
    expect_eq(decodeEbmlUint(u9[0..]) == null, true);

    // Element declares size=50 in a 4-byte buffer -> terminates safely with null
    const trunc: [4]u8 = [0x42, 0x82, 0xB2, 'w'];
    let it = EbmlElementIter.init(trunc[0..]);
    expect_eq(it.next() == null, true);
});
