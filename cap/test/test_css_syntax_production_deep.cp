// Source: cp/cap/test/test_css_syntax_production_deep.cp (Production deep tests for cap:css_syntax)

import { test, expect_eq } from "cap:test";
import {
    SyntaxString,
    SyntaxComponent,
    SyntaxType,
    Multiplier,
} from "cap:css";

test("Universal '*' syntax parsing and serialization") {
    const alloc = std.testing.allocator;
    const syn = SyntaxString.parse(alloc, "  *  ") catch unreachable;
    expect_eq(syn.isUniversal(), true);
    expect_eq(syn.len(), @as(usize, 0));

    let buf: [16]u8 = [0; 16];
    expect_eq(syn.toCss(buf[0..16]) catch unreachable, "*");
    expect_eq(syn.toCssQuoted(buf[0..16]) catch unreachable, "\"*\"");
}

test("All 15 standard CSS syntax types parsing and case-insensitive normalization") {
    const alloc = std.testing.allocator;
    const type_inputs: [15]string = [
        "<length>",
        "<number>",
        "<percentage>",
        "<length-percentage>",
        "<color>",
        "<image>",
        "<url>",
        "<integer>",
        "<angle>",
        "<time>",
        "<resolution>",
        "<transform-function>",
        "<transform-list>",
        "<custom-ident>",
        "<string>",
    ];

    let buf: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 15) {
        const syn = SyntaxString.parse(alloc, type_inputs[i]) catch unreachable;
        expect_eq(syn.isUniversal(), false);
        expect_eq(syn.len(), @as(usize, 1));
        expect_eq(syn.items()[0].multiplier == .none, true);
        expect_eq(syn.toCss(buf[0..64]) catch unreachable, type_inputs[i]);
        i += 1;
    }

    // 大小写无关解析并规范化为小写输出
    const upper = SyntaxString.parse(alloc, "<LENGTH-PERCENTAGE>") catch unreachable;
    expect_eq(upper.items()[0].kind == .length_percentage, true);
    expect_eq(upper.toCss(buf[0..64]) catch unreachable, "<length-percentage>");
}

test("Multipliers none/space/comma and <transform-list> multiplier prohibition") {
    const alloc = std.testing.allocator;

    const s_plus = SyntaxString.parse(alloc, "<length>+") catch unreachable;
    expect_eq(s_plus.items()[0].kind == .length, true);
    expect_eq(s_plus.items()[0].multiplier == .space, true);
    expect_eq(s_plus.items()[0].multiplier.asStr(), "+");

    const s_hash = SyntaxString.parse(alloc, "<color>#") catch unreachable;
    expect_eq(s_hash.items()[0].kind == .color, true);
    expect_eq(s_hash.items()[0].multiplier == .comma, true);
    expect_eq(s_hash.items()[0].multiplier.asStr(), "#");

    // 预乘类型 <transform-list> 合法无乘数，但禁止 '+' 或 '#'
    const tl_ok = SyntaxString.parse(alloc, "<transform-list>") catch unreachable;
    expect_eq(tl_ok.items()[0].kind == .transform_list, true);
    expect_eq(SyntaxType.transform_list.allowsMultiplier(), false);

    const tl_plus = switch (SyntaxString.parse(alloc, "<transform-list>+")) {
        err.MultiplierNotAllowed => true,
        else => false,
    };
    expect_eq(tl_plus, true);

    const tl_hash = switch (SyntaxString.parse(alloc, "<transform-list>#")) {
        err.MultiplierNotAllowed => true,
        else => false,
    };
    expect_eq(tl_hash, true);
}

test("Pipe combinator multi-branch syntax with literals and canonical serialization") {
    const alloc = std.testing.allocator;
    const raw = " <length>+   | <color># |   auto | -custom-kw ";
    const syn = SyntaxString.parse(alloc, raw) catch unreachable;

    expect_eq(syn.len(), @as(usize, 4));
    const items = syn.items();
    expect_eq(items[0].eql(SyntaxComponent{ kind: .length, multiplier: .space }), true);
    expect_eq(items[1].eql(SyntaxComponent{ kind: .color, multiplier: .comma }), true);
    expect_eq(items[2].eql(SyntaxComponent{ kind: .literal, literal: "auto", multiplier: .none }), true);
    expect_eq(items[3].eql(SyntaxComponent{ kind: .literal, literal: "-custom-kw", multiplier: .none }), true);

    let buf: [128]u8 = [0; 128];
    const canonical = syn.toCss(buf[0..128]) catch unreachable;
    expect_eq(canonical, "<length>+ | <color># | auto | -custom-kw");

    const quoted = syn.toCssQuoted(buf[0..128]) catch unreachable;
    expect_eq(quoted, "\"<length>+ | <color># | auto | -custom-kw\"");
}

test("Malformed syntax, empty branches, trailing pipe and buffer bounds guards") {
    const alloc = std.testing.allocator;

    expect_eq(switch (SyntaxString.parse(alloc, "")) { err.EmptySyntax => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "   \t\n ")) { err.EmptySyntax => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "<length")) { err.UnclosedTypeName => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "<unknown-type>")) { err.UnknownTypeName => true, else => false }, true);

    // 前导 |、尾随 |、连续 ||
    expect_eq(switch (SyntaxString.parse(alloc, "| <color>")) { err.InvalidComponent => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "<color> |")) { err.InvalidComponent => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "<color> | | <length>")) { err.InvalidComponent => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "123invalid")) { err.InvalidComponent => true, else => false }, true);

    // 缺少 | 分隔或双乘数
    expect_eq(switch (SyntaxString.parse(alloc, "<length> <color>")) { err.UnexpectedToken => true, else => false }, true);
    expect_eq(switch (SyntaxString.parse(alloc, "<length>++")) { err.UnexpectedToken => true, else => false }, true);

    // 序列化缓冲过小 -> BufferTooSmall
    const syn = SyntaxString.parse(alloc, "<length> | <color>") catch unreachable;
    let tiny: [5]u8 = [0; 5];
    expect_eq(switch (syn.toCss(tiny[0..5])) { err.BufferTooSmall => true, else => false }, true);
}

test("Parse-serialize-reparse idempotency and RAII zero-leak verification") {
    const alloc = std.testing.allocator;
    const cases: [4]string = [
        "*",
        "<angle> | <time>+",
        "none | <image># | <url>",
        "<integer>+ | <number> | <percentage>#",
    ];

    let buf1: [128]u8 = [0; 128];
    let buf2: [128]u8 = [0; 128];
    let i: usize = 0;
    while (i < 4) {
        const s1 = SyntaxString.parse(alloc, cases[i]) catch unreachable;
        const out1 = s1.toCss(buf1[0..128]) catch unreachable;
        const s2 = SyntaxString.parse(alloc, out1) catch unreachable;
        const out2 = s2.toCss(buf2[0..128]) catch unreachable;
        expect_eq(out1, out2);
        expect_eq(s1.len(), s2.len());
        i += 1;
    }
}
