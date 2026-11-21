// Source: cp/cap/test/test_smol_str_production_deep.cp (Production deep tests for cap:smol_str)

import { test, expect_eq } from "cap:test";
import { SmolStr, INLINE_MAX_LEN } from "cap:smol_str";

test("Empty and inline strings (<= 15 bytes) use zero heap allocation") {
    expect_eq(INLINE_MAX_LEN, @as(usize, 15));

    const empty = SmolStr.empty();
    expect_eq(empty.isEmpty(), true);
    expect_eq(empty.isInline(), true);
    expect_eq(empty.isHeap(), false);
    expect_eq(empty.len(), @as(usize, 0));
    expect_eq(empty.slice(), "");

    const one = SmolStr.fromChar('Z');
    expect_eq(one.isInline(), true);
    expect_eq(one.len(), @as(usize, 1));
    expect_eq(one.slice(), "Z");

    // 恰好 15 字节边界仍为内联存储
    const max_inline = SmolStr.fromInline("123456789012345") catch unreachable;
    expect_eq(max_inline.isInline(), true);
    expect_eq(max_inline.isHeap(), false);
    expect_eq(max_inline.len(), @as(usize, 15));
    expect_eq(max_inline.slice(), "123456789012345");

    // 16 字节调用 fromInline 必须返回 StringTooLong
    const too_long = switch (SmolStr.fromInline("1234567890123456")) {
        err.StringTooLong => true,
        else => false,
    };
    expect_eq(too_long, true);
}

test("Strings exceeding inline threshold transparently allocate on heap") {
    const alloc = std.testing.allocator;

    // 15 字节经 fromSlice 依然内联
    const s15 = SmolStr.fromSlice(alloc, "abcdefghijklmno") catch unreachable;
    expect_eq(s15.isInline(), true);
    expect_eq(s15.isHeap(), false);
    expect_eq(s15.len(), @as(usize, 15));

    // 16 字节透明转堆
    const s16 = SmolStr.fromSlice(alloc, "abcdefghijklmnop") catch unreachable;
    expect_eq(s16.isInline(), false);
    expect_eq(s16.isHeap(), true);
    expect_eq(s16.len(), @as(usize, 16));
    expect_eq(s16.slice(), "abcdefghijklmnop");

    // 长串 128 字节
    let long_buf: [128]u8 = ['K'; 128];
    long_buf[0] = 'A';
    long_buf[127] = 'Z';
    const s128 = SmolStr.fromSlice(alloc, long_buf[0..128]) catch unreachable;
    expect_eq(s128.isHeap(), true);
    expect_eq(s128.len(), @as(usize, 128));
    expect_eq(s128.eqlStr(long_buf[0..128]), true);
}

test("eql, eqlStr and hash consistency across inline and heap storage") {
    const alloc = std.testing.allocator;

    const a = SmolStr.fromInline("cp-lang") catch unreachable;
    const b = SmolStr.fromSlice(alloc, "cp-lang") catch unreachable;
    const c = SmolStr.fromInline("cp-rust") catch unreachable;

    expect_eq(a.eql(b), true);
    expect_eq(a.hash(), b.hash());
    expect_eq(a.eql(c), false);
    expect_eq(a.hash() != c.hash(), true);

    const h1 = SmolStr.fromSlice(alloc, "long-string-for-hash-check-0001") catch unreachable;
    const h2 = SmolStr.fromSlice(alloc, "long-string-for-hash-check-0001") catch unreachable;
    const h3 = SmolStr.fromSlice(alloc, "long-string-for-hash-check-0002") catch unreachable;
    expect_eq(h1.eql(h2), true);
    expect_eq(h1.hash(), h2.hash());
    expect_eq(h1.eql(h3), false);
    expect_eq(h1.hash() != h3.hash(), true);
}

test("appendChar and appendSlice seamless inline-to-heap promotion and growth") {
    const alloc = std.testing.allocator;
    let s = SmolStr.empty();

    // 追加到 15 字节：保持内联
    s.appendSlice(alloc, "0123456789") catch unreachable;
    expect_eq(s.isInline(), true);
    s.appendSlice(alloc, "abcde") catch unreachable;
    expect_eq(s.isInline(), true);
    expect_eq(s.len(), @as(usize, 15));
    expect_eq(s.slice(), "0123456789abcde");

    // 第 16 字节触发内联 -> 堆透明迁移
    s.appendChar(alloc, '!') catch unreachable;
    expect_eq(s.isInline(), false);
    expect_eq(s.isHeap(), true);
    expect_eq(s.len(), @as(usize, 16));
    expect_eq(s.slice(), "0123456789abcde!");

    // 多次追加触发堆缓冲倍增扩容
    let round: usize = 0;
    while (round < 10) {
        s.appendSlice(alloc, "_segment") catch unreachable;
        round += 1;
    }
    expect_eq(s.len(), @as(usize, 16 + 80));
    expect_eq(std.mem.startsWith(u8, s.slice(), "0123456789abcde!_segment"), true);
    expect_eq(std.mem.endsWith(u8, s.slice(), "_segment"), true);
}

test("clone creates independent deep copies for both inline and heap strings") {
    const alloc = std.testing.allocator;

    const orig_inline = SmolStr.fromSlice(alloc, "short") catch unreachable;
    let copy_inline = orig_inline.clone(alloc) catch unreachable;
    expect_eq(copy_inline.isInline(), true);
    expect_eq(copy_inline.eql(orig_inline), true);
    copy_inline.appendSlice(alloc, "_extended_beyond_15_bytes") catch unreachable;
    expect_eq(copy_inline.isHeap(), true);
    expect_eq(orig_inline.isInline(), true);
    expect_eq(orig_inline.slice(), "short");

    const orig_heap = SmolStr.fromSlice(alloc, "heap-allocated-initial-content") catch unreachable;
    let copy_heap = orig_heap.clone(alloc) catch unreachable;
    expect_eq(copy_heap.isHeap(), true);
    expect_eq(copy_heap.eql(orig_heap), true);
    copy_heap.appendSlice(alloc, "+mutated") catch unreachable;
    expect_eq(orig_heap.slice(), "heap-allocated-initial-content");
    expect_eq(copy_heap.slice(), "heap-allocated-initial-content+mutated");
}

test("Idempotent ~init() RAII cleanup and multi-round zero-leak verification") {
    const alloc = std.testing.allocator;

    let i: usize = 0;
    while (i < 16) {
        let s = SmolStr.fromSlice(alloc, "iteration-payload-exceeding-15-bytes") catch unreachable;
        s.appendChar(alloc, 'X') catch unreachable;
        expect_eq(s.isHeap(), true);
        if (i % 2 == 0) {
            // 验证显式调用 ~init() 后离开作用域不触发 Double-Free
            s.~init();
        }
        i += 1;
    }
}
