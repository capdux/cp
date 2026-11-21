import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SfItemKind,
    SfParamIter,
    parseSfItem,
    formatSfString,
    formatSfByteSeq,
} from "cap:structured_headers";

test("1. parseSfItem parses boolean, integer (up to 15 digits), and decimal_milli (up to 3 frac digits)", () => {
    let used: usize = 0;

    const b1 = parseSfItem("?1", &used) ?? unreachable;
    expect_eq(b1.kind == SfItemKind.boolean, true);
    expect_eq(b1.bool_val, true);
    expect_eq(used, usize(2));

    const b0 = parseSfItem("?0", &used) ?? unreachable;
    expect_eq(b0.kind == SfItemKind.boolean, true);
    expect_eq(b0.bool_val, false);

    const i1 = parseSfItem("-999999999999999", &used) ?? unreachable;
    expect_eq(i1.kind == SfItemKind.integer, true);
    expect_eq(i1.int_val, i64(-999999999999999));

    const d1 = parseSfItem("1.25", &used) ?? unreachable;
    expect_eq(d1.kind == SfItemKind.decimal_milli, true);
    expect_eq(d1.int_val, i64(1250));

    const d2 = parseSfItem("-0.005", &used) ?? unreachable;
    expect_eq(d2.kind == SfItemKind.decimal_milli, true);
    expect_eq(d2.int_val, i64(-5));

    // RFC 8941 上限拒绝：16 位整数、4 位小数、无小数位点
    expect_eq(parseSfItem("1000000000000000", &used) == null, true);
    expect_eq(parseSfItem("1.2345", &used) == null, true);
    expect_eq(parseSfItem("1.", &used) == null, true);
});

test("2. parseSfItem parses strings, tokens, and byte sequences", () => {
    let used: usize = 0;

    const s = parseSfItem("\"hello \\\"cp\\\" \\\\\"", &used) ?? unreachable;
    expect_eq(s.kind == SfItemKind.string, true);
    expect_eq(std.mem.eql(u8, s.str_val, "hello \\\"cp\\\" \\\\"), true);

    const tok = parseSfItem("text/html:v2", &used) ?? unreachable;
    expect_eq(tok.kind == SfItemKind.token, true);
    expect_eq(std.mem.eql(u8, tok.str_val, "text/html:v2"), true);

    const bs = parseSfItem(":cHJldGVuZCB0aGlzIGlzIGJpbmFyeQ==:", &used) ?? unreachable;
    expect_eq(bs.kind == SfItemKind.byte_seq, true);
    expect_eq(std.mem.eql(u8, bs.str_val, "cHJldGVuZCB0aGlzIGlzIGJpbmFyeQ=="), true);

    // 拒绝非法转义、未闭合字符串与非法 base64 字节序列
    expect_eq(parseSfItem("\"bad\\n\"", &used) == null, true);
    expect_eq(parseSfItem("\"unclosed", &used) == null, true);
    expect_eq(parseSfItem(":not!b64:", &used) == null, true);
});

test("3. SfParamIter parses HTTP Priority / Cache-Status style parameter lists", () => {
    const hdr = "u=3;i;q=0.75;label=\"edge\"";
    let used: usize = 0;
    // 先取前导 token，或直接测纯参数段
    const full = "hit;ttl=60;collapsed;detail=\"mem\";ratio=1.5";
    const head = parseSfItem(full, &used) ?? unreachable;
    expect_eq(head.kind == SfItemKind.token, true);
    expect_eq(std.mem.eql(u8, head.str_val, "hit"), true);

    let it = SfParamIter.init(full[used..]);

    const p0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p0.key, "ttl"), true);
    expect_eq(p0.val.kind == SfItemKind.integer, true);
    expect_eq(p0.val.int_val, i64(60));

    // 无 = 的参数默认为 boolean(true)
    const p1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.key, "collapsed"), true);
    expect_eq(p1.val.kind == SfItemKind.boolean, true);
    expect_eq(p1.val.bool_val, true);

    const p2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.key, "detail"), true);
    expect_eq(p2.val.kind == SfItemKind.string, true);
    expect_eq(std.mem.eql(u8, p2.val.str_val, "mem"), true);

    const p3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p3.key, "ratio"), true);
    expect_eq(p3.val.kind == SfItemKind.decimal_milli, true);
    expect_eq(p3.val.int_val, i64(1500));

    expect_eq(it.next() == null, true);
    expect_eq(it.hasError(), false);
    * = hdr;
});

test("4. formatSfString and formatSfByteSeq formatting and validation", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const s = formatSfString("a\"b\\c", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, s, "\"a\\\"b\\\\c\""), true);
    // 拒绝控制字符
    expect_eq(formatSfString("a\nb", &buf) == null, true);

    const bs = formatSfByteSeq("AQID", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, bs, ":AQID:"), true);
    expect_eq(formatSfByteSeq("bad space", &buf) == null, true);
});

test("5. SfParamIter error flags and buffer-too-small guards", () => {
    // 大写参数键按 RFC 8941 非法
    let bad_it = SfParamIter.init(";BadKey=1");
    expect_eq(bad_it.next() == null, true);
    expect_eq(bad_it.hasError(), true);

    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    expect_eq(formatSfString("abc", &tiny) == null, true);
    expect_eq(formatSfByteSeq("AQID", &tiny) == null, true);
});
