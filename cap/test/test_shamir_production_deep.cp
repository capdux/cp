// Source: cp/cap/test/test_shamir_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ShamirError,
    Share,
    freeShares,
    splitSecret,
    combineShares,
} from "cap:crypto";

test("1. (3, 5) threshold secret splitting and arbitrary subset recovery") {
    const alloc = std.testing.allocator;
    const secret = "master-ed25519-root-seed-2026-cp-lang!";
    const shares = try splitSecret(alloc, secret, 3, 5);
    defer freeShares(alloc, shares);
    expect_eq(shares.len, 5);

    // Subset {0, 2, 4}
    const sub1: [3]Share = [ shares[0], shares[2], shares[4] ];
    const rec1 = try combineShares(alloc, &sub1);
    defer alloc.free(rec1);
    try std.testing.expectEqualStrings(secret, rec1);

    // Subset {1, 3, 4}
    const sub2: [3]Share = [ shares[1], shares[3], shares[4] ];
    const rec2 = try combineShares(alloc, &sub2);
    defer alloc.free(rec2);
    try std.testing.expectEqualStrings(secret, rec2);
}

test("2. Share wire encode/decode roundtrip and cross-subset combine") {
    const alloc = std.testing.allocator;
    const secret = "0123456789ABCDEF";
    const shares = try splitSecret(alloc, secret, 2, 4);
    defer freeShares(alloc, shares);

    const enc0 = try shares[0].encode(alloc);
    defer alloc.free(enc0);
    const enc3 = try shares[3].encode(alloc);
    defer alloc.free(enc3);

    const dec0 = try Share.decode(alloc, enc0);
    defer dec0.deinit(alloc);
    const dec3 = try Share.decode(alloc, enc3);
    defer dec3.deinit(alloc);

    const pair: [2]Share = [ dec0, dec3 ];
    const recovered = try combineShares(alloc, &pair);
    defer alloc.free(recovered);
    try std.testing.expectEqualStrings(secret, recovered);
}

test("3. Fewer than threshold shares cannot recover secret") {
    const alloc = std.testing.allocator;
    const secret = "top-secret-key-material-32bytes!";
    const shares = try splitSecret(alloc, secret, 3, 4);
    defer freeShares(alloc, shares);

    const too_few: [2]Share = [ shares[0], shares[1] ];
    try std.testing.expectError(ShamirError.InsufficientShares, combineShares(alloc, &too_few));
}

test("4. Duplicate share index, zero index, length mismatch and threshold validation") {
    const alloc = std.testing.allocator;
    try std.testing.expectError(ShamirError.EmptySecret, splitSecret(alloc, "", 2, 3));
    try std.testing.expectError(ShamirError.InvalidThreshold, splitSecret(alloc, "abc", 1, 3));
    try std.testing.expectError(ShamirError.InvalidThreshold, splitSecret(alloc, "abc", 4, 3));

    const shares = try splitSecret(alloc, "hello", 2, 3);
    defer freeShares(alloc, shares);
    const dup: [2]Share = [ shares[0], shares[0] ];
    try std.testing.expectError(ShamirError.DuplicateShareIndex, combineShares(alloc, &dup));

    try std.testing.expectError(ShamirError.InvalidShareFormat, Share.decode(alloc, "BAD1"));
}

test("5. Dynamic cap.shamir namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    const shares = try cap.crypto.splitSecret(alloc, "cap-shamir-ok", 2, 3);
    defer cap.crypto.freeShares(alloc, shares);
    const pair: [2]cap.crypto.Share = [ shares[1], shares[2] ];
    const out = try cap.crypto.combineShares(alloc, &pair);
    defer alloc.free(out);
    try std.testing.expectEqualStrings("cap-shamir-ok", out);
}
