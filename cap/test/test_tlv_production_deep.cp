// Source: cp/cap/test/test_tlv_production_deep.cp
// Deep Production Test Suite for cap:tlv (ASN.1 BER/DER TLV)
// Covers:
//   1. DER nested SEQUENCE/SET and high-tag-number (Base-128) encode/decode round-trip
//   2. Bidirectional lossless OID dotted string <-> DER OID octets conversion
//   3. TlvWriter minimal two's-complement integer normalization (0x00/0xFF trimming & 0x00 sign padding)
//   4. Strict DER guards: NonCanonicalLength, IndefiniteLengthNotAllowed (0x80), MaxDepthExceeded, UnexpectedEof, TrailingBytes
//   5. Dynamic namespace mounting via `import cap from "cap"; cap.tlv` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TLV_MAX_DEPTH,
    TAG_NUM_SEQUENCE,
    TAG_NUM_SET,
    TAG_NUM_OID,
    TlvError,
    TagClass,
    TlvTag,
    TlvNode,
    TlvWriter,
    decodeDer,
    decodeDerWithMaxDepth,
    encodeDerNode,
    encodeOidBytes,
    decodeOid,
} from "cap:tlv";
import cap from "cap";

fn testSequenceSetAndHighTagRoundTrip()! {
    const alloc = std.testing.allocator;

    let w = TlvWriter.init(alloc);
    defer w.deinit();

    // Outer SEQUENCE
    w.beginSequence()?;
    w.writeBoolean(true)?;
    w.writeBoolean(false)?;
    w.writeOctetString("\xDE\xAD\xBE\xEF")?;
    w.writeUtf8String("cp-asn1-der")?;

    // Nested SET
    w.beginSet()?;
    // High-tag-number ContextSpecific constructed (tag 0x1234 = 4660)
    const high_tag = TlvTag.contextSpecific(4660, true);
    w.beginConstructed(high_tag)?;
    // High-tag-number Application primitive (tag 31)
    const app_tag = TlvTag.application(31, false);
    w.writeRaw(app_tag, "hi-tag-31")?;
    w.endConstructed()?;
    w.endConstructed()?;

    w.endConstructed()?;

    const der_bytes = w.toOwnedSlice()?;
    defer alloc.free(der_bytes);

    let root = decodeDer(alloc, der_bytes)?;
    defer root.deinit(alloc);

    expect_eq(root.tag.number, TAG_NUM_SEQUENCE);
    expect_eq(root.tag.constructed, true);
    expect_eq(root.children.len, 5 as usize);
    expect_eq(root.children[0].asBoolean()?, true);
    expect_eq(root.children[1].asBoolean()?, false);
    expect_eq(std.mem.eql(u8, root.children[2].asOctetString()?, "\xDE\xAD\xBE\xEF"), true);
    expect_eq(std.mem.eql(u8, root.children[3].asUtf8String()?, "cp-asn1-der"), true);

    const set_node = root.children[4];
    expect_eq(set_node.tag.number, TAG_NUM_SET);
    const high_node = set_node.findChild(4660).?;
    expect_eq(high_node.tag.constructed, true);
    const app_node = high_node.findChild(31).?;
    expect_eq(app_node.tag.constructed, false);
    expect_eq(std.mem.eql(u8, app_node.raw_value, "hi-tag-31"), true);

    // Re-encode decoded TlvNode tree and verify byte-for-byte DER identity
    const reencoded = encodeDerNode(alloc, root)?;
    defer alloc.free(reencoded);
    expect_eq(std.mem.eql(u8, der_bytes, reencoded), true);
}

fn testOidBidirectionalConversion()! {
    const alloc = std.testing.allocator;

    // sha256WithRSAEncryption: 1.2.840.113549.1.1.11
    const sha256_rsa = "1.2.840.113549.1.1.11";
    const expected_sha256_rsa = "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x0B";

    let w = TlvWriter.init(alloc);
    defer w.deinit();
    w.writeOid(sha256_rsa)?;

    const der_oid = w.toOwnedSlice()?;
    defer alloc.free(der_oid);

    let node = decodeDer(alloc, der_oid)?;
    defer node.deinit(alloc);
    expect_eq(node.tag.number, TAG_NUM_OID);
    expect_eq(std.mem.eql(u8, node.raw_value, expected_sha256_rsa), true);

    const decoded_str = decodeOid(alloc, node.raw_value)?;
    defer alloc.free(decoded_str);
    expect_eq(std.mem.eql(u8, decoded_str, sha256_rsa), true);

    // Arc0 = 2 with large arc1: 2.999.123456
    const large_oid = "2.999.123456";
    const raw_large = encodeOidBytes(alloc, large_oid)?;
    defer alloc.free(raw_large);
    const decoded_large = decodeOid(alloc, raw_large)?;
    defer alloc.free(decoded_large);
    expect_eq(std.mem.eql(u8, decoded_large, large_oid), true);

    // Invalid OID guard (arc0 > 2)
    switch (encodeOidBytes(alloc, "3.1.2")) {
        v => { alloc.free(v); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.InvalidOid, true),
    }
}

fn testIntegerMinimalNormalization()! {
    const alloc = std.testing.allocator;

    let w = TlvWriter.init(alloc);
    defer w.deinit();

    w.beginSequence()?;
    // Redundant leading 0x00s for positive integer 0x7F -> 02 01 7F
    w.writeIntegerBytes("\x00\x00\x7F")?;
    // Redundant leading 0xFFs for negative integer -128 -> 02 01 80
    w.writeIntegerBytes("\xFF\xFF\x80")?;
    // Unsigned integer with MSB=1 (0x80 0x01) -> needs 0x00 sign prefix -> 02 03 00 80 01
    w.writeUnsignedIntegerBytes("\x00\x00\x80\x01")?;
    // Empty integer -> 02 01 00
    w.writeIntegerBytes("")?;
    w.endConstructed()?;

    const der = w.toOwnedSlice()?;
    defer alloc.free(der);

    let root = decodeDer(alloc, der)?;
    defer root.deinit(alloc);

    expect_eq(root.children.len, 4 as usize);
    expect_eq(std.mem.eql(u8, root.children[0].asIntegerBytes()?, "\x7F"), true);
    expect_eq(root.children[0].asU64()?, 127 as u64);

    expect_eq(std.mem.eql(u8, root.children[1].asIntegerBytes()?, "\x80"), true);
    expect_eq(std.mem.eql(u8, root.children[2].asIntegerBytes()?, "\x00\x80\x01"), true);
    expect_eq(root.children[2].asU64()?, 0x8001 as u64);
    expect_eq(std.mem.eql(u8, root.children[3].asIntegerBytes()?, "\x00"), true);
    expect_eq(root.children[3].asU64()?, 0 as u64);

    // Non-minimal integer in raw DER must be rejected by asIntegerBytes()
    const bad_int_der = "\x02\x02\x00\x7F";
    let bad_node = decodeDer(alloc, bad_int_der)?;
    defer bad_node.deinit(alloc);
    switch (bad_node.asIntegerBytes()) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.InvalidInteger, true),
    }
}

fn testStrictDerGuards()! {
    const alloc = std.testing.allocator;

    // 1. Indefinite length 0x80 -> IndefiniteLengthNotAllowed
    switch (decodeDer(alloc, "\x30\x80\x00\x00")) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.IndefiniteLengthNotAllowed, true),
    }

    // 2. Non-canonical length (< 128 encoded in long form: 0x81 0x05) -> NonCanonicalLength
    switch (decodeDer(alloc, "\x04\x81\x05hello")) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.NonCanonicalLength, true),
    }

    // 3. Long form with redundant leading 0x00 (0x82 0x00 0x80) -> NonCanonicalLength
    switch (decodeDer(alloc, "\x04\x82\x00\x80")) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.NonCanonicalLength, true),
    }

    // 4. Truncated payload -> UnexpectedEof
    switch (decodeDer(alloc, "\x04\x05abc")) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.UnexpectedEof, true),
    }

    // 5. Trailing bytes after root element -> TrailingBytes
    switch (decodeDer(alloc, "\x05\x00\x00")) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.TrailingBytes, true),
    }

    // 6. MaxDepthExceeded (3 nested SEQUENCEs with max_depth = 1)
    const nested3 = "\x30\x04\x30\x02\x30\x00";
    switch (decodeDerWithMaxDepth(alloc, nested3, 1)) {
        n => { n.deinit(alloc); expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.MaxDepthExceeded, true),
    }

    // 7. InvalidBoolean (0x01 instead of 0xFF)
    let bad_bool = decodeDer(alloc, "\x01\x01\x01")?;
    defer bad_bool.deinit(alloc);
    switch (bad_bool.asBoolean()) {
        v => { * = v; expect_eq(true, false); },
        err(e) => expect_eq(e == TlvError.InvalidBoolean, true),
    }
}

fn testDynamicCapTlvNamespace()! {
    const alloc = std.testing.allocator;
    expect_eq(cap.tlv.TLV_MAX_DEPTH, TLV_MAX_DEPTH);

    let w = cap.tlv.TlvWriter.init(alloc);
    defer w.deinit();
    w.beginSequence()?;
    w.writeOid("1.3.6.1.5.5.7.1.1")?;
    w.writeUtf8String("ok")?;
    w.endConstructed()?;

    const raw = w.toOwnedSlice()?;
    defer alloc.free(raw);

    let root = cap.tlv.decodeDer(alloc, raw)?;
    defer root.deinit(alloc);

    const oid_str = cap.tlv.decodeOid(alloc, root.children[0].raw_value)?;
    defer alloc.free(oid_str);
    expect_eq(std.mem.eql(u8, oid_str, "1.3.6.1.5.5.7.1.1"), true);
    expect_eq(std.mem.eql(u8, root.children[1].asUtf8String()?, "ok"), true);

    const reencoded = cap.tlv.encodeDerNode(alloc, root)?;
    defer alloc.free(reencoded);
    expect_eq(std.mem.eql(u8, raw, reencoded), true);
}

test("tlv: DER SEQUENCE/SET & high-tag-number round-trip", () => {
    try testSequenceSetAndHighTagRoundTrip();
});

test("tlv: OID dotted string <-> DER octets bidirectional conversion", () => {
    try testOidBidirectionalConversion();
});

test("tlv: TlvWriter minimal two's-complement integer normalization", () => {
    try testIntegerMinimalNormalization();
});

test("tlv: strict DER guards (NonCanonicalLength, 0x80, MaxDepthExceeded, EOF, TrailingBytes)", () => {
    try testStrictDerGuards();
});

test("tlv: dynamic cap.tlv namespace mounting & zero-leak", () => {
    try testDynamicCapTlvNamespace();
});
