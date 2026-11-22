// Source: cp/cap/test/test_stun_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:stun & cap.stun

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAGIC_COOKIE,
    FINGERPRINT_XOR,
    StunError,
    StunClass,
    StunMethod,
    AttrType,
    StunAddr,
    XorMappedAddress,
    StunErrorCode,
    StunMessage,
    StunBuilder,
    parseMessage,
    verifyMessageIntegrity,
    verifyFingerprint,
    decodeXorMappedAddress,
    decodeErrorCode,
} from "cap:net";

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const tid: [12]u8 = [0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06];

    // 1. Binding Request (empty body)
    let req_builder = StunBuilder.init(alloc, StunClass.Request, StunMethod.Binding as u16, tid);
    defer req_builder.deinit();
    const req_wire = req_builder.finish(alloc)?;
    defer alloc.free(req_wire);

    expect_eq(req_wire.len, 20 as usize);
    // Binding Request = 0x0001
    expect_eq(req_wire[0], 0x00 as u8);
    expect_eq(req_wire[1], 0x01 as u8);
    // Magic Cookie = 0x2112A442
    expect_eq(req_wire[4], 0x21 as u8);
    expect_eq(req_wire[5], 0x12 as u8);
    expect_eq(req_wire[6], 0xA4 as u8);
    expect_eq(req_wire[7], 0x42 as u8);

    let parsed_req = parseMessage(alloc, req_wire)?;
    defer parsed_req.deinit(alloc);
    expect_eq(parsed_req.msg_class == StunClass.Request, true);
    expect_eq(parsed_req.method, StunMethod.Binding as u16);
    expect_eq(std.mem.eql(u8, &parsed_req.transaction_id, &tid), true);
    expect_eq(parsed_req.attributes.len, 0 as usize);

    // 2. Binding Success Response with IPv4 XOR-MAPPED-ADDRESS
    let resp4_builder = StunBuilder.init(alloc, StunClass.SuccessResponse, StunMethod.Binding as u16, tid);
    defer resp4_builder.deinit();
    resp4_builder.addXorMappedAddress(StunAddr{ ipv4: [192, 0, 2, 1] }, 32853)?;
    const resp4_wire = resp4_builder.finish(alloc)?;
    defer alloc.free(resp4_wire);

    // Binding Success Response = 0x0101
    expect_eq(resp4_wire[0], 0x01 as u8);
    expect_eq(resp4_wire[1], 0x01 as u8);

    let parsed_resp4 = parseMessage(alloc, resp4_wire)?;
    defer parsed_resp4.deinit(alloc);
    expect_eq(parsed_resp4.msg_class == StunClass.SuccessResponse, true);
    const xattr4 = parsed_resp4.findAttr(AttrType.XorMappedAddress as u16) orelse unreachable;
    // Verify raw XOR-encoded port: 32853 (0x8055) ^ 0x2112 = 0xA147
    expect_eq(xattr4.value[2], 0xA1 as u8);
    expect_eq(xattr4.value[3], 0x47 as u8);
    const decoded4 = decodeXorMappedAddress(xattr4.value, parsed_resp4.transaction_id)?;
    expect_eq(decoded4.port, 32853 as u16);
    switch (decoded4.addr) {
        StunAddr.ipv4 => |ip4| {
            expect_eq(ip4[0], 192 as u8);
            expect_eq(ip4[1], 0 as u8);
            expect_eq(ip4[2], 2 as u8);
            expect_eq(ip4[3], 1 as u8);
        },
        StunAddr.ipv6 => unreachable,
    }

    // 3. Binding Success Response with IPv6 XOR-MAPPED-ADDRESS
    const ip6_expected: [16]u8 = [
        0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00,
        0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34,
    ];
    let resp6_builder = StunBuilder.init(alloc, StunClass.SuccessResponse, StunMethod.Binding as u16, tid);
    defer resp6_builder.deinit();
    resp6_builder.addXorMappedAddress(StunAddr{ ipv6: ip6_expected }, 49152)?;
    const resp6_wire = resp6_builder.finish(alloc)?;
    defer alloc.free(resp6_wire);

    let parsed_resp6 = parseMessage(alloc, resp6_wire)?;
    defer parsed_resp6.deinit(alloc);
    const xattr6 = parsed_resp6.findAttr(AttrType.XorMappedAddress as u16) orelse unreachable;
    const decoded6 = decodeXorMappedAddress(xattr6.value, parsed_resp6.transaction_id)?;
    expect_eq(decoded6.port, 49152 as u16);
    switch (decoded6.addr) {
        StunAddr.ipv6 => |ip6| {
            expect_eq(std.mem.eql(u8, &ip6, &ip6_expected), true);
        },
        StunAddr.ipv4 => unreachable,
    }
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const tid: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
    const key = "VOkJHz2/14/ice/password";

    let builder = StunBuilder.init(alloc, StunClass.Request, StunMethod.Binding as u16, tid);
    defer builder.deinit();
    builder.addAttribute(AttrType.Username as u16, "ufrag1:ufrag2")?;
    builder.addXorMappedAddress(StunAddr{ ipv4: [203, 0, 113, 9] }, 4433)?;

    const wire = builder.finishWithIntegrity(alloc, key, true)?;
    defer alloc.free(wire);

    expect_eq(verifyMessageIntegrity(wire, key)?, true);
    expect_eq(verifyFingerprint(wire)?, true);

    let msg = parseMessage(alloc, wire)?;
    defer msg.deinit(alloc);
    expect_eq(msg.findAttr(AttrType.MessageIntegrity as u16) != null, true);
    expect_eq(msg.findAttr(AttrType.Fingerprint as u16) != null, true);

    // Wrong key must fail MESSAGE-INTEGRITY
    let wrong_key_err = false;
    * = verifyMessageIntegrity(wire, "wrong-password") catch |e| {
        if (e == StunError.IntegrityMismatch) {
            wrong_key_err = true;
        }
    };
    expect_eq(wrong_key_err, true);

    // Bit-flip in FINGERPRINT must fail FingerprintMismatch
    let fp_tampered = alloc.dupe(u8, wire) catch unreachable;
    defer alloc.free(fp_tampered);
    fp_tampered[fp_tampered.len - 1] ^= 0x01;
    let fp_err = false;
    * = verifyFingerprint(fp_tampered) catch |e| {
        if (e == StunError.FingerprintMismatch) {
            fp_err = true;
        }
    };
    expect_eq(fp_err, true);

    // Bit-flip in payload attribute must fail both IntegrityMismatch and FingerprintMismatch
    let body_tampered = alloc.dupe(u8, wire) catch unreachable;
    defer alloc.free(body_tampered);
    body_tampered[25] ^= 0x80;
    let mi_err = false;
    * = verifyMessageIntegrity(body_tampered, key) catch |e| {
        if (e == StunError.IntegrityMismatch) {
            mi_err = true;
        }
    };
    expect_eq(mi_err, true);

    let fp_body_err = false;
    * = verifyFingerprint(body_tampered) catch |e| {
        if (e == StunError.FingerprintMismatch) {
            fp_body_err = true;
        }
    };
    expect_eq(fp_body_err, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    const tid: [12]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC];

    let builder = StunBuilder.init(alloc, StunClass.ErrorResponse, StunMethod.Binding as u16, tid);
    defer builder.deinit();

    // Non-4-byte aligned attributes: 5 bytes ("alice"), 7 bytes ("cp-stun")
    builder.addAttribute(AttrType.Username as u16, "alice")?;
    builder.addAttribute(AttrType.Software as u16, "cp-stun")?;
    builder.addErrorCode(401, "Unauthorized")?;

    const wire = builder.finish(alloc)?;
    defer alloc.free(wire);

    // Total length must be 4-byte aligned
    expect_eq((wire.len % 4) as usize, 0 as usize);

    let msg = parseMessage(alloc, wire)?;
    defer msg.deinit(alloc);
    expect_eq(msg.msg_class == StunClass.ErrorResponse, true);

    const user_attr = msg.findAttr(AttrType.Username as u16) orelse unreachable;
    expect_eq(std.mem.eql(u8, user_attr.value, "alice"), true);

    const sw_attr = msg.findAttr(AttrType.Software as u16) orelse unreachable;
    expect_eq(std.mem.eql(u8, sw_attr.value, "cp-stun"), true);

    const ec_attr = msg.findAttr(AttrType.ErrorCode as u16) orelse unreachable;
    const ec = decodeErrorCode(ec_attr.value)?;
    expect_eq(ec.code, 401 as u16);
    expect_eq(std.mem.eql(u8, ec.reason, "Unauthorized"), true);

    // Invalid error code (< 300) must fail
    let bad_code = false;
    builder.addErrorCode(200, "OK") catch |e| {
        if (e == StunError.InvalidAttributeValue) {
            bad_code = true;
        }
    };
    expect_eq(bad_code, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    const tid: [12]u8 = [0; 12];

    let builder = StunBuilder.init(alloc, StunClass.Request, StunMethod.Binding as u16, tid);
    defer builder.deinit();
    builder.addAttribute(AttrType.Software as u16, "test")?;
    const valid_wire = builder.finish(alloc)?;
    defer alloc.free(valid_wire);

    // 1. Top 2 bits non-zero -> InvalidHeader
    let bad_bits = alloc.dupe(u8, valid_wire) catch unreachable;
    defer alloc.free(bad_bits);
    bad_bits[0] |= 0x80;
    let got_hdr_err = false;
    * = parseMessage(alloc, bad_bits) catch |e| {
        if (e == StunError.InvalidHeader) {
            got_hdr_err = true;
        }
    };
    expect_eq(got_hdr_err, true);

    // 2. Invalid Magic Cookie -> InvalidMagicCookie
    let bad_cookie = alloc.dupe(u8, valid_wire) catch unreachable;
    defer alloc.free(bad_cookie);
    bad_cookie[4] ^= 0xFF;
    let got_cookie_err = false;
    * = parseMessage(alloc, bad_cookie) catch |e| {
        if (e == StunError.InvalidMagicCookie) {
            got_cookie_err = true;
        }
    };
    expect_eq(got_cookie_err, true);

    // 3. Length not a multiple of 4 -> InvalidMessageLength
    let bad_len = alloc.dupe(u8, valid_wire) catch unreachable;
    defer alloc.free(bad_len);
    bad_len[3] = 5;
    let got_len_err = false;
    * = parseMessage(alloc, bad_len) catch |e| {
        if (e == StunError.InvalidMessageLength) {
            got_len_err = true;
        }
    };
    expect_eq(got_len_err, true);

    // 4. Truncated attribute (attr_len exceeds message body) -> TruncatedAttribute
    let trunc_attr = alloc.dupe(u8, valid_wire) catch unreachable;
    defer alloc.free(trunc_attr);
    // valid_wire has 1 attribute of 4 bytes (header 20 + attr_hdr 4 + val 4 = 28). Set attr_len = 20.
    trunc_attr[22] = 0;
    trunc_attr[23] = 20;
    let got_trunc_err = false;
    * = parseMessage(alloc, trunc_attr) catch |e| {
        if (e == StunError.TruncatedAttribute) {
            got_trunc_err = true;
        }
    };
    expect_eq(got_trunc_err, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    expect_eq(cap.net.MAGIC_COOKIE, MAGIC_COOKIE);
    expect_eq(cap.net.FINGERPRINT_XOR, FINGERPRINT_XOR);

    const tid: [12]u8 = [9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2];
    let b = cap.net.StunBuilder.init(
        alloc,
        cap.net.StunClass.Indication,
        cap.net.StunMethod.Binding as u16,
        tid,
    );
    defer b.deinit();
    b.addAttribute(cap.net.AttrType.Username as u16, "dyn-mount")?;
    const wire = b.finishWithIntegrity(alloc, "dyn-key", true)?;
    defer alloc.free(wire);

    expect_eq(cap.net.verifyMessageIntegrity(wire, "dyn-key")?, true);
    expect_eq(cap.net.verifyFingerprint(wire)?, true);

    let msg = cap.net.parseMessage(alloc, wire)?;
    defer msg.deinit(alloc);
    expect_eq(msg.msg_class == cap.net.StunClass.Indication, true);
    const u = msg.findAttr(cap.net.AttrType.Username as u16) orelse unreachable;
    expect_eq(std.mem.eql(u8, u.value, "dyn-mount"), true);
}

test("cap:stun Test 1: Binding Request & Success Response (IPv4/IPv6 XOR-MAPPED-ADDRESS)") {
    runTest1() catch unreachable;
}

test("cap:stun Test 2: MESSAGE-INTEGRITY (HMAC-SHA1) & FINGERPRINT (CRC32 ^ 0x5354554e) verification & tampering") {
    runTest2() catch unreachable;
}

test("cap:stun Test 3: Error Response (ERROR-CODE 300..699) & 4-byte unaligned attribute padding") {
    runTest3() catch unreachable;
}

test("cap:stun Test 4: Header top-2-bits, Magic Cookie, length alignment & TruncatedAttribute rejection") {
    runTest4() catch unreachable;
}

test("cap:stun Test 5: Dynamic cap.stun mount & zero memory leak") {
    runTest5() catch unreachable;
}
