// Source: cp/cap/test/test_bech32_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Bech32Variant,
    Bech32Error,
    encode,
    decode,
    encodeBytes,
    decodeBytes,
    encodeSegwitAddr,
} from "cap:crypto";

test("1. BIP-173 & BIP-350 official specification test vectors") {
    const alloc = std.testing.allocator;
    const d1 = try decode(alloc, "A12UEL5L");
    defer d1.deinit(alloc);
    try std.testing.expectEqualStrings("a", d1.hrp);
    expect_eq(d1.variant, Bech32Variant.bech32);

    const d2 = try decode(alloc, "A1LQFN3A");
    defer d2.deinit(alloc);
    try std.testing.expectEqualStrings("a", d2.hrp);
    expect_eq(d2.variant, Bech32Variant.bech32m);

    const empty5: [0]u8 = [];
    const e1 = try encode(alloc, "a", &empty5, .bech32);
    defer alloc.free(e1);
    try std.testing.expectEqualStrings("a12uel5l", e1);

    const e2 = try encode(alloc, "a", &empty5, .bech32m);
    defer alloc.free(e2);
    try std.testing.expectEqualStrings("a1lqfn3a", e2);
}

test("2. SegWit v0 (Bech32) and v1 Taproot (Bech32m) official vectors") {
    const alloc = std.testing.allocator;
    const prog0: [20]u8 = [
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
        0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6,
    ];
    const addr0 = try encodeSegwitAddr(alloc, "bc", 0, &prog0);
    defer alloc.free(addr0);
    try std.testing.expectEqualStrings("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4", addr0);

    const prog1: [32]u8 = [
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
    ];
    const addr1 = try encodeSegwitAddr(alloc, "bc", 1, &prog1);
    defer alloc.free(addr1);
    try std.testing.expectEqualStrings("bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0", addr1);
}

test("3. Arbitrary bytes encodeBytes and decodeBytes (Nostr/Age style)") {
    const alloc = std.testing.allocator;
    const raw = "cp-bech32-32byte-public-key-seed";
    const encoded = try encodeBytes(alloc, "npub", raw, .bech32);
    defer alloc.free(encoded);
    try std.testing.expect(std.mem.startsWith(u8, encoded, "npub1"));

    const out = try decodeBytes(alloc, encoded);
    defer out.deinit(alloc);
    try std.testing.expectEqualStrings("npub", out.hrp);
    try std.testing.expectEqualStrings(raw, out.bytes);
}

test("4. Checksum tampering, mixed case and invalid witness protection") {
    const alloc = std.testing.allocator;
    try std.testing.expectError(Bech32Error.MixedCase, decode(alloc, "A12uel5l"));
    try std.testing.expectError(Bech32Error.InvalidChecksum, decode(alloc, "a12uel5m"));
    try std.testing.expectError(Bech32Error.MissingSeparator, decode(alloc, "abcdef123456"));
    try std.testing.expectError(Bech32Error.InvalidCharacter, decode(alloc, "a12uel5b"));

    const short_prog: [3]u8 = [ 1, 2, 3 ];
    try std.testing.expectError(Bech32Error.InvalidWitnessProgram, encodeSegwitAddr(alloc, "bc", 0, &short_prog));
}

test("5. Dynamic cap.bech32 namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    const encoded = try cap.crypto.encodeBytes(alloc, "cp", "hello-bech32m", .bech32m);
    defer alloc.free(encoded);
    const out = try cap.crypto.decodeBytes(alloc, encoded);
    defer out.deinit(alloc);
    try std.testing.expectEqualStrings("hello-bech32m", out.bytes);
    expect_eq(out.variant, cap.crypto.Bech32Variant.bech32m);
}
