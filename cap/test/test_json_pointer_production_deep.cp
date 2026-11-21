// Source: cp/cap/test/test_json_pointer_production_deep.cp (Production deep tests for cap:json_pointer)

import { test, expect_eq } from "cap:test";
import {
    isValidPointer,
    parseArrayIndex,
    unescapeToken,
    escapeToken,
    PointerIter,
    appendToken,
    parentPointer,
} from "cap:json_pointer";

test("json_pointer: isValidPointer RFC 6901 syntax validation") {
    expect_eq(isValidPointer(""), true);
    expect_eq(isValidPointer("/"), true);
    expect_eq(isValidPointer("/foo/0"), true);
    expect_eq(isValidPointer("/a~1b/m~0n"), true);
    expect_eq(isValidPointer("/~01"), true); // 先 ~0 -> ~，后 1 -> ~1，合法

    // 非法：非空未以 '/' 开头、孤立 '~'、非法 '~2'/'~a'
    expect_eq(isValidPointer("foo"), false);
    expect_eq(isValidPointer("a/b"), false);
    expect_eq(isValidPointer("/foo~"), false);
    expect_eq(isValidPointer("/foo~2bar"), false);
    expect_eq(isValidPointer("/~a"), false);
}

test("json_pointer: parseArrayIndex RFC 6901 §4 rules and leading zero / overflow guards") {
    expect_eq(parseArrayIndex("0").?, 0);
    expect_eq(parseArrayIndex("1").?, 1);
    expect_eq(parseArrayIndex("42").?, 42);
    expect_eq(parseArrayIndex("9876543210").?, 9876543210);

    // 拒绝空串、前导零、负号、追加符 '-'、非数字与超大溢出
    expect_eq(parseArrayIndex("") == null, true);
    expect_eq(parseArrayIndex("00") == null, true);
    expect_eq(parseArrayIndex("01") == null, true);
    expect_eq(parseArrayIndex("-1") == null, true);
    expect_eq(parseArrayIndex("-") == null, true);
    expect_eq(parseArrayIndex("12a") == null, true);
    expect_eq(parseArrayIndex("99999999999999999999999999999") == null, true);
}

test("json_pointer: escapeToken and unescapeToken round-trip and RFC 6901 ~01 precedence") {
    let esc_buf: [64]u8 = [0; 64];
    let unesc_buf: [64]u8 = [0; 64];

    const raw = "a/b~c/~1";
    const en = escapeToken(raw, esc_buf[0..]).?;
    expect_eq(std.mem.eql(u8, esc_buf[0..en], "a~1b~0c~1~01"), true);

    const un = unescapeToken(esc_buf[0..en], unesc_buf[0..]).?;
    expect_eq(std.mem.eql(u8, unesc_buf[0..un], raw), true);

    // RFC 6901 §4: "~01" 必须解码为 "~1"（不可二次解码为 "/"）
    const n01 = unescapeToken("~01", unesc_buf[0..]).?;
    expect_eq(std.mem.eql(u8, unesc_buf[0..n01], "~1"), true);

    // 非法转义与小缓冲溢出防护
    expect_eq(unescapeToken("bad~", unesc_buf[0..]) == null, true);
    expect_eq(unescapeToken("bad~9", unesc_buf[0..]) == null, true);

    let tiny: [2]u8 = [0; 2];
    expect_eq(escapeToken("a/b", tiny[0..]) == null, true);
    expect_eq(unescapeToken("abc", tiny[0..]) == null, true);
}

test("json_pointer: PointerIter raw/unescaped iteration, empty key segment and remaining()") {
    let it = PointerIter.init("/users/0/a~1b/m~0n/").?;
    let buf: [64]u8 = [0; 64];

    const s0 = it.nextRaw().?;
    expect_eq(std.mem.eql(u8, s0, "users"), true);
    expect_eq(std.mem.eql(u8, it.remaining(), "/0/a~1b/m~0n/"), true);

    const s1 = it.nextRaw().?;
    expect_eq(parseArrayIndex(s1).?, 0);

    const s2 = it.nextUnescaped(buf[0..]).?;
    expect_eq(std.mem.eql(u8, s2, "a/b"), true);

    const s3 = it.nextUnescaped(buf[0..]).?;
    expect_eq(std.mem.eql(u8, s3, "m~n"), true);

    // 末尾 '/' 代表空字符串键 ""
    const s4 = it.nextRaw().?;
    expect_eq(s4.len, 0);
    expect_eq(it.remaining().len, 0);
    expect_eq(it.nextRaw() == null, true);

    // 根指针 "" 零段迭代；非法指针 init 返回 null
    let root_it = PointerIter.init("").?;
    expect_eq(root_it.nextRaw() == null, true);
    expect_eq(PointerIter.init("missing-slash") == null, true);
}

test("json_pointer: appendToken and parentPointer navigation") {
    let buf1: [64]u8 = [0; 64];
    let buf2: [64]u8 = [0; 64];

    const n1 = appendToken("", "a/b", buf1[0..]).?;
    const p1 = buf1[0..n1];
    expect_eq(std.mem.eql(u8, p1, "/a~1b"), true);

    const n2 = appendToken(p1, "c~d", buf2[0..]).?;
    const p2 = buf2[0..n2];
    expect_eq(std.mem.eql(u8, p2, "/a~1b/c~0d"), true);

    // parentPointer 逐层向上直到根
    const par1 = parentPointer(p2).?;
    expect_eq(std.mem.eql(u8, par1, "/a~1b"), true);

    const par2 = parentPointer(par1).?;
    expect_eq(par2.len, 0);

    expect_eq(parentPointer(par2) == null, true);
    expect_eq(parentPointer("invalid") == null, true);

    // appendToken 非法 base 与小缓冲防护
    expect_eq(appendToken("bad", "x", buf1[0..]) == null, true);
    let tiny: [3]u8 = [0; 3];
    expect_eq(appendToken("/a", "b/c", tiny[0..]) == null, true);
}
