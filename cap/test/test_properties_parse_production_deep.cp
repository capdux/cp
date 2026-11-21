// Source: Absorbed from /data/dev/bun/src/ini.zig (Java .properties Zero-Alloc Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PropertyEntry,
    PropertiesIter,
    getProperty,
} from "cap:properties_parse";

test("1. PropertiesIter parses =, :, whitespace separators, comments (# and !), blank lines, and empty values", () => {
    const src =
        "# hash comment\n"
        ++ "   ! bang comment\n"
        ++ "\n"
        ++ "host = localhost\n"
        ++ "port: 8080\n"
        ++ "mode   production\n"
        ++ "empty_only\n"
        ++ "empty_eq =\n"
        ++ "after_cr=ok\r\n";

    let kbuf: [64]u8 = std.mem.zeroes([64]u8);
    let vbuf: [128]u8 = std.mem.zeroes([128]u8);
    let it = PropertiesIter.init(src);

    const e1 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "host"), true);
    expect_eq(std.mem.eql(u8, e1.value, "localhost"), true);
    expect_eq(e1.line_no, 4);

    const e2 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "port"), true);
    expect_eq(std.mem.eql(u8, e2.value, "8080"), true);
    expect_eq(e2.line_no, 5);

    const e3 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "mode"), true);
    expect_eq(std.mem.eql(u8, e3.value, "production"), true);
    expect_eq(e3.line_no, 6);

    const e4 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "empty_only"), true);
    expect_eq(e4.value.len, 0);
    expect_eq(e4.line_no, 7);

    const e5 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e5.key, "empty_eq"), true);
    expect_eq(e5.value.len, 0);
    expect_eq(e5.line_no, 8);

    const e6 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e6.key, "after_cr"), true);
    expect_eq(std.mem.eql(u8, e6.value, "ok"), true);
    expect_eq(e6.line_no, 9);

    expect_eq(it.next(&kbuf, &vbuf) == null, true);
});

test("2. Escaped delimiters inside keys and logical line continuations (odd vs even backslashes)", () => {
    const src =
        "key\\=with\\:delims\\ space = val1\n"
        ++ "multi = hello \\\n"
        ++ "    world\\\r\n"
        ++ "\t!\n"
        ++ "even_slash = C:\\\\\n"
        ++ "next_key = yes\n"
        ++ "odd_slash = A\\\\\\\n"
        ++ "   B\n";

    let kbuf: [64]u8 = std.mem.zeroes([64]u8);
    let vbuf: [128]u8 = std.mem.zeroes([128]u8);
    let it = PropertiesIter.init(src);

    const e1 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "key=with:delims space"), true);
    expect_eq(std.mem.eql(u8, e1.value, "val1"), true);
    expect_eq(e1.line_no, 1);

    const e2 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "multi"), true);
    expect_eq(std.mem.eql(u8, e2.value, "hello world!"), true);
    expect_eq(e2.line_no, 2);

    // 偶数个反斜杠不触发续行
    const e3 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "even_slash"), true);
    expect_eq(std.mem.eql(u8, e3.value, "C:\\"), true);
    expect_eq(e3.line_no, 5);

    const e4 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "next_key"), true);
    expect_eq(std.mem.eql(u8, e4.value, "yes"), true);
    expect_eq(e4.line_no, 6);

    // 奇数个反斜杠：前两个产出字面 \，第三个续行
    const e5 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e5.key, "odd_slash"), true);
    expect_eq(std.mem.eql(u8, e5.value, "A\\B"), true);
    expect_eq(e5.line_no, 7);
});

test("3. Escape sequences \\n \\r \\t \\f \\\\ and \\uXXXX (including UTF-16 surrogate pairs) decode to UTF-8", () => {
    const src =
        "ctrl = a\\nb\\rc\\td\\fe\\\\g\n"
        ++ "uni = \\u0041\\u00E9\\u4E2D\\uD83D\\uDE80\n"
        ++ "literal = \\#notcomment\\!ok\n";

    let kbuf: [64]u8 = std.mem.zeroes([64]u8);
    let vbuf: [128]u8 = std.mem.zeroes([128]u8);
    let it = PropertiesIter.init(src);

    const e1 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.value, "a\nb\rc\td\fe\\g"), true);

    const e2 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.value, "Aé中🚀"), true);

    const e3 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.value, "#notcomment!ok"), true);
});

test("4. getProperty applies last-wins override and skips unrelated oversized values", () => {
    const src =
        "\xEF\xBB\xBF"
        ++ "dup = first\n"
        ++ "huge = this_value_is_way_longer_than_eight_bytes_buffer\n"
        ++ "dup : second\n"
        ++ "only = hi\n";

    let tiny: [8]u8 = std.mem.zeroes([8]u8);

    const v_dup = getProperty(src, "dup", &tiny) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_dup, "second"), true);

    const v_only = getProperty(src, "only", &tiny) ?? unreachable;
    expect_eq(std.mem.eql(u8, v_only, "hi"), true);

    expect_eq(getProperty(src, "missing", &tiny) == null, true);
    // 目标值超过 val_buf 返回 null
    expect_eq(getProperty(src, "huge", &tiny) == null, true);
});

test("5. Malformed unicode escapes, lone surrogates, and tiny key/val buffer guards", () => {
    const src =
        "bad1 = \\u12G4\n"
        ++ "bad2 = \\u12\n"
        ++ "bad3 = \\uD800\n"
        ++ "bad4 = \\uDC00\n"
        ++ "good = ok\n";

    let kbuf: [32]u8 = std.mem.zeroes([32]u8);
    let vbuf: [32]u8 = std.mem.zeroes([32]u8);
    let it = PropertiesIter.init(src);

    // 畸形 unicode 转义行被安全跳过，不影响后续合法条目
    const e1 = it.next(&kbuf, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "good"), true);
    expect_eq(std.mem.eql(u8, e1.value, "ok"), true);
    expect_eq(it.next(&kbuf, &vbuf) == null, true);

    // key_buf 过小时安全跳过而不越界
    let tiny_k: [2]u8 = std.mem.zeroes([2]u8);
    let it2 = PropertiesIter.init("longkey=1\nab=2\n");
    const e2 = it2.next(&tiny_k, &vbuf) ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "ab"), true);
    expect_eq(std.mem.eql(u8, e2.value, "2"), true);
});
