// Source: cp/cap/test/test_reed_solomon_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RsError,
    gfMul,
    gfInv,
    gfDiv,
    ReedSolomon,
} from "cap:codec";

test("1. GF(2^8) arithmetic and multiplicative inverse axioms") {
    let a: u16 = 1;
    while (a < 256) : (a += 1) {
        const ua: u8 = @intCast(a);
        const inv = gfInv(ua);
        expect_eq(gfMul(ua, inv), 1);
        expect_eq(gfDiv(ua, ua), 1);
    }
    expect_eq(gfMul(0x57, 0x83), 0xc1);
}

test("2. RS(4, 2) systematic encoding and verification") {
    const alloc = std.testing.allocator;
    const rs = try ReedSolomon.init(alloc, 4, 2);
    defer rs.deinit();

    let s0: [16]u8 = [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 ];
    let s1: [16]u8 = [ 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36 ];
    let s2: [16]u8 = [ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 ];
    let s3: [16]u8 = [ 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76 ];
    let p0: [16]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    let p1: [16]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    const shards: [6][]u8 = [ &s0, &s1, &s2, &s3, &p0, &p1 ];

    try rs.encode(&shards);
    expect_eq(try rs.verify(alloc, &shards), true);

    // Flip 1 byte in parity -> verify must return false
    p1[7] ^= 0x01;
    expect_eq(try rs.verify(alloc, &shards), false);
}

test("3. RS(4, 3) lossless reconstruction of mixed data and parity shard losses") {
    const alloc = std.testing.allocator;
    const rs = try ReedSolomon.init(alloc, 4, 3);
    defer rs.deinit();

    let orig0: [12]u8 = [ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 ];
    let orig2: [12]u8 = [ 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 1, 2, 3, 4, 5, 6 ];
    let s0 = orig0;
    let s1: [12]u8 = [ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 255, 254 ];
    let s2 = orig2;
    let s3: [12]u8 = [ 13, 26, 39, 52, 65, 78, 91, 104, 117, 130, 143, 156 ];
    let p0: [12]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    let p1: [12]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    let p2: [12]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    const shards: [7][]u8 = [ &s0, &s1, &s2, &s3, &p0, &p1, &p2 ];

    try rs.encode(&shards);

    // Wipe out data shards 0, 2 and parity shard 1 (total 3 losses == parity_shards)
    @memset(&s0, 0);
    @memset(&s2, 0);
    @memset(&p1, 0);
    const present: [7]bool = [ false, true, false, true, true, false, true ];

    try rs.reconstruct(alloc, &shards, &present);
    try std.testing.expectEqualSlices(u8, &orig0, &s0);
    try std.testing.expectEqualSlices(u8, &orig2, &s2);
    expect_eq(try rs.verify(alloc, &shards), true);
}

test("4. Error boundaries: TooFewShards, InvalidShardCount and ShardSizeMismatch") {
    const alloc = std.testing.allocator;
    try std.testing.expectError(RsError.InvalidShardCount, ReedSolomon.init(alloc, 0, 2));
    try std.testing.expectError(RsError.InvalidShardCount, ReedSolomon.init(alloc, 200, 60));

    const rs = try ReedSolomon.init(alloc, 2, 1);
    defer rs.deinit();
    let a: [4]u8 = [ 1, 2, 3, 4 ];
    let b: [4]u8 = [ 5, 6, 7, 8 ];
    let c: [4]u8 = [ 0, 0, 0, 0 ];
    const shards: [3][]u8 = [ &a, &b, &c ];
    const too_few: [3]bool = [ false, false, true ];
    try std.testing.expectError(RsError.TooFewShards, rs.reconstruct(alloc, &shards, &too_few));

    let bad: [3]u8 = [ 1, 2, 3 ];
    const mis: [3][]u8 = [ &a, &bad, &c ];
    try std.testing.expectError(RsError.ShardSizeMismatch, rs.encode(&mis));
}

test("5. Dynamic cap.reed_solomon namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    const rs = try cap.codec.ReedSolomon.init(alloc, 2, 1);
    defer rs.deinit();
    let d0: [4]u8 = [ 0x11, 0x22, 0x33, 0x44 ];
    let d1: [4]u8 = [ 0x55, 0x66, 0x77, 0x88 ];
    let p0: [4]u8 = [ 0, 0, 0, 0 ];
    const shards: [3][]u8 = [ &d0, &d1, &p0 ];
    try rs.encode(&shards);
    @memset(&d0, 0);
    const present: [3]bool = [ false, true, true ];
    try rs.reconstruct(alloc, &shards, &present);
    expect_eq(d0[0], 0x11);
    expect_eq(d0[3], 0x44);
}
