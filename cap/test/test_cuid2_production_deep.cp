// Source: cp/cap/test/test_cuid2_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { CUID2_DEFAULT_LEN, Cuid2Error, Cuid2Generator, isValidCuid2 } from "cap:codec";

test "cuid2: default 24-char format and lowercase alpha prefix" {
    let gen = Cuid2Generator.init("node-a");
    const id1 = gen.next24();
    const id2 = gen.next24();
    try expect_eq(id1.len, CUID2_DEFAULT_LEN);
    try expect_eq(isValidCuid2(&id1), true);
    try expect_eq(isValidCuid2(&id2), true);
    try expect_eq(std.mem.eql(u8, &id1, &id2), false);
}

test "cuid2: custom lengths 4..32 and InvalidLength guard" {
    let gen = Cuid2Generator.init("node-b");
    let short_buf: [4]u8 = undefined;
    const s4 = try gen.nextInto(&short_buf);
    try expect_eq(s4.len, 4);
    try expect_eq(isValidCuid2(s4), true);

    let long_buf: [32]u8 = undefined;
    const s32 = try gen.nextInto(&long_buf);
    try expect_eq(s32.len, 32);
    try expect_eq(isValidCuid2(s32), true);

    let bad_short: [3]u8 = undefined;
    try expect_eq(gen.nextInto(&bad_short) == Cuid2Error.InvalidLength, true);
}

test "cuid2: uniqueness across 100 generated IDs" {
    const alloc = std.testing.allocator;
    let gen = Cuid2Generator.init("batch-test");
    let map = std.StringHashMap(void).init(alloc);
    defer {
        let it = map.keyIterator();
        while (k := it.next()) {
            alloc.free(k.*);
        }
        map.deinit();
    }

    let i: usize = 0;
    while (i < 100) : (i += 1) {
        const id = gen.next24();
        const dup = try alloc.dupe(u8, &id);
        const gop = try map.getOrPut(dup);
        if (gop.found_existing) {
            alloc.free(dup);
            return error.DuplicateId;
        }
    }
    try expect_eq(map.count(), 100);
}

test "cuid2: isValidCuid2 rejects uppercase, symbols, and digit-prefix" {
    try expect_eq(isValidCuid2("1abcdefg"), false);
    try expect_eq(isValidCuid2("Abcdefgh"), false);
    try expect_eq(isValidCuid2("abc-defg"), false);
    try expect_eq(isValidCuid2("abc"), false);
    try expect_eq(isValidCuid2("tz4a98xxat96iws9zmbrgj3a"), true);
}

test "cuid2: cap.cuid2 dynamic mount" {
    let gen = cap.codec.Cuid2Generator.init("dyn-mount");
    const id = gen.next24();
    try expect_eq(cap.codec.isValidCuid2(&id), true);
}
