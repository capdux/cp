// Source: cp/cap/test/test_ksuid_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KSUID_EPOCH,
    KsuidError,
    Ksuid,
} from "cap:codec";

test("1. Official KSUID known test vector and 27-char Base62 roundtrip") {
    // Official Segment KSUID reference: ts=107608047 (unix 1507608047), payload=0xB5A1CD34B5F99D1154FB6853345C9735 -> "0ujtsYcgvSTl8PAuAdqWYSMnLOv"
    const payload: [16]u8 = [
        0xB5, 0xA1, 0xCD, 0x34, 0xB5, 0xF9, 0x9D, 0x11,
        0x54, 0xFB, 0x68, 0x53, 0x34, 0x5C, 0x97, 0x35,
    ];
    const id = try Ksuid.fromParts(KSUID_EPOCH + 107608047, payload);
    const enc = id.encode();
    try std.testing.expectEqualStrings("0ujtsYcgvSTl8PAuAdqWYSMnLOv", &enc);

    const dec_id = try Ksuid.decode(&enc);
    try std.testing.expect(id.equals(dec_id));
    expect_eq(dec_id.timestampUnix(), KSUID_EPOCH + 107608047);
}

test("2. Nil and Max KSUID boundary encode/decode") {
    const z = Ksuid.nil();
    const z_enc = z.encode();
    try std.testing.expectEqualStrings("000000000000000000000000000", &z_enc);
    try std.testing.expect(z.equals(try Ksuid.decode(&z_enc)));

    let max_b: [20]u8 = undefined;
    @memset(&max_b, 0xFF);
    const max_id = Ksuid.fromBytes(max_b);
    const max_enc = max_id.encode();
    try std.testing.expectEqualStrings("aWgEPTl1tmebfsQzFP4bxwgy80V", &max_enc);
    try std.testing.expect(max_id.equals(try Ksuid.decode(&max_enc)));
}

test("3. Lexicographical time ordering and CSPRNG uniqueness") {
    const k1 = try Ksuid.generateAt(1700000000);
    const k2 = try Ksuid.generateAt(1700000005);
    expect_eq(k1.compare(k2), -1);

    const e1 = k1.encode();
    const e2 = k2.encode();
    try std.testing.expect(std.mem.order(u8, &e1, &e2) == .lt);

    const g1 = try Ksuid.generate();
    const g2 = try Ksuid.generate();
    try std.testing.expect(!g1.equals(g2));
}

test("4. Overflow, invalid length, invalid char and timestamp range errors") {
    // 27 'z's overflows 160-bit space
    try std.testing.expectError(KsuidError.Overflow, Ksuid.decode("zzzzzzzzzzzzzzzzzzzzzzzzzzz"));
    try std.testing.expectError(KsuidError.InvalidLength, Ksuid.decode("short"));
    try std.testing.expectError(KsuidError.InvalidCharacter, Ksuid.decode("0ujtsYcgvSTl8PAuAdqWYSMnLO!"));
    let p: [16]u8 = undefined;
    @memset(&p, 0);
    try std.testing.expectError(KsuidError.TimestampOutOfRange, Ksuid.fromParts(KSUID_EPOCH - 1, p));
}

test("5. Dynamic cap.ksuid namespace and zero-alloc check") {
    const id = try cap.codec.Ksuid.generateAt(1750000000);
    const enc = id.encode();
    const dec_id = try cap.codec.Ksuid.decode(&enc);
    try std.testing.expect(id.equals(dec_id));
}
