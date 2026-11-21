// Source: cp/cap/test/test_small_string_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { MAX_INLINE_LEN, SmallString, ExternalString, StringInternPool } from "cap:small_string";

test "small_string: 8-byte inline SSO vs arena-backed long strings" {
    try expect_eq(@sizeOf(SmallString), 8);
    try expect_eq(MAX_INLINE_LEN, 8);

    const s_short = SmallString.initInline("1.2.3");
    try expect_eq(s_short.isInline(), true);
    try expect_eq(s_short.len(), 5);
    try expect_eq(std.mem.eql(u8, s_short.slice(""), "1.2.3"), true);

    const s_8 = SmallString.initInline("12345678");
    try expect_eq(s_8.isInline(), true);
    try expect_eq(s_8.len(), 8);
}

test "small_string: StringInternPool deduplicates long strings & saves arena bytes" {
    const alloc = std.testing.allocator;
    let pool = StringInternPool.init(alloc);
    defer pool.deinit();

    const a = try pool.intern("@scope/very-long-package-name");
    const b = try pool.intern("@scope/very-long-package-name");
    const c = try pool.intern("lodash"); // <= 8 bytes -> inline, 0 arena bytes

    try expect_eq(c.value.isInline(), true);
    try expect_eq(a.value.isInline(), false);
    try expect_eq(a.hash, b.hash);
    try expect_eq(pool.buffer().len, 29);
    try expect_eq(std.mem.eql(u8, a.slice(pool.buffer()), "@scope/very-long-package-name"), true);
}

test "small_string: order & eql comparison" {
    const alloc = std.testing.allocator;
    let pool = StringInternPool.init(alloc);
    defer pool.deinit();

    const s1 = try pool.intern("alpha");
    const s2 = try pool.intern("beta-long-name");
    try expect_eq(s1.value.order(&s2.value, pool.buffer(), pool.buffer()) == .lt, true);
    try expect_eq(s1.eql(&s2, pool.buffer(), pool.buffer()), false);
}

test "small_string: cap.small_string dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let pool = cap.small_string.StringInternPool.init(alloc);
    defer pool.deinit();
    const x = try pool.intern("cap-stdlib-test");
    try expect_eq(std.mem.eql(u8, x.slice(pool.buffer()), "cap-stdlib-test"), true);
}
