// Source: cp/cap/test/test_grapheme_cluster_production_deep.cp (Production deep tests for cap:grapheme_cluster)

import { test, expect_eq } from "cap:test";
import {
    GraphemeBreakProperty,
    GraphemeBreakState,
    GraphemeIterator,
    classifyCodepoint,
    shouldBreak,
    decodeUtf8Step,
    count,
    sliceByGraphemes,
} from "cap:grapheme_cluster";

test("ASCII, CRLF vs LFCR and control character boundaries (GB3/GB4/GB5)") {
    expect_eq(count(""), @as(usize, 0));
    expect_eq(count("hello"), @as(usize, 5));

    // GB3: \r\n 合并为 1 个字素簇；\n\r 拆为 2 个
    expect_eq(count("a\r\nb"), @as(usize, 3));
    expect_eq(count("a\n\rb"), @as(usize, 4));

    let it = GraphemeIterator.init("a\r\nb");
    expect_eq(std.mem.eql(u8, it.next().?, "a"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "\r\n"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "b"), true);
    expect_eq(it.next() == null, true);

    expect_eq(classifyCodepoint(0x000D) == .CR, true);
    expect_eq(classifyCodepoint(0x000A) == .LF, true);
    expect_eq(classifyCodepoint(0x0000) == .Control, true);
}

test("Combining marks (e + acute) and Hangul Jamo composition (GB6-GB9)") {
    // "e\u{0301}" = 0x65 0xCC 0x81 -> 1 个字素簇
    const e_acute: [3]u8 = [0x65, 0xCC, 0x81];
    expect_eq(count(e_acute[0..3]), @as(usize, 1));

    // "a\u{0300}\u{0323}" (多组合音标) + "z" -> 2 个字素簇
    const multi_comb: [6]u8 = [0x61, 0xCC, 0x80, 0xCC, 0xA3, 0x7A];
    expect_eq(count(multi_comb[0..6]), @as(usize, 2));

    // Hangul L (U+1100) + V (U+1161) + T (U+11A8) -> 1 个字素簇
    let st = GraphemeBreakState.init();
    expect_eq(shouldBreak(*st, 0x1100, 0x1161), false); // L × V
    expect_eq(shouldBreak(*st, 0x1161, 0x11A8), false); // V × T
    expect_eq(shouldBreak(*st, 0x11A8, 0x0041), true);  // T ÷ 'A'

    // Precomposed Hangul LV (U+AC00) × T (U+11A8) -> 不折断
    st.reset();
    expect_eq(classifyCodepoint(0xAC00) == .HangulLV, true);
    expect_eq(classifyCodepoint(0xAC01) == .HangulLVT, true);
    expect_eq(shouldBreak(*st, 0xAC00, 0x11A8), false);
    expect_eq(shouldBreak(*st, 0xAC01, 0x11A8), false);
}

test("Emoji ZWJ family sequence, skin tone modifiers and variation selectors (GB11)") {
    // 👨‍👩‍👧‍👦 = U+1F468 ZWJ U+1F469 ZWJ U+1F467 ZWJ U+1F466 (25 UTF-8 bytes -> 1 grapheme cluster!)
    const family = "👨‍👩‍👧‍👦";
    expect_eq(count(family), @as(usize, 1));

    let it = GraphemeIterator.init(family);
    const first = it.next().?;
    expect_eq(first.len, family.len);
    expect_eq(it.next() == null, true);

    // 肤色修饰符 👍🏽 (U+1F44D + U+1F3FD) -> 1 个字素簇
    const thumbs_up_med = "👍🏽";
    expect_eq(count(thumbs_up_med), @as(usize, 1));

    // 变体选择符 ❤️ (U+2764 + U+FE0F) -> 1 个字素簇
    const red_heart = "❤️";
    expect_eq(count(red_heart), @as(usize, 1));

    // 混合序列："A👨‍👩‍👧‍👦👍🏽❤️B" -> 5 个字素簇
    const mixed = "A👨‍👩‍👧‍👦👍🏽❤️B";
    expect_eq(count(mixed), @as(usize, 5));
}

test("Regional Indicator flag pairs (CN + US) and odd RI sequence (GB12/GB13)") {
    // 🇨🇳🇺🇸 = 4 个 RI 码点 (16 字节) -> 精确切分为 2 个国旗字素簇
    const flags = "🇨🇳🇺🇸";
    expect_eq(count(flags), @as(usize, 2));

    let it = GraphemeIterator.init(flags);
    expect_eq(std.mem.eql(u8, it.next().?, "🇨🇳"), true);
    expect_eq(std.mem.eql(u8, it.next().?, "🇺🇸"), true);
    expect_eq(it.next() == null, true);

    // 3 个连续 RI：前 2 个配对成 1 簇，第 3 个独立成 1 簇
    const three_ri = "🇨🇳🇺";
    expect_eq(count(three_ri), @as(usize, 2));
}

test("sliceByGraphemes zero-allocation slicing and out-of-bounds guards") {
    const s = "Hi👨‍👩‍👧‍👦🇨🇳你好!";
    expect_eq(count(s), @as(usize, 7));

    expect_eq(std.mem.eql(u8, sliceByGraphemes(s, 0, 2), "Hi"), true);
    expect_eq(std.mem.eql(u8, sliceByGraphemes(s, 2, 4), "👨‍👩‍👧‍👦🇨🇳"), true);
    expect_eq(std.mem.eql(u8, sliceByGraphemes(s, 4, 6), "你好"), true);
    expect_eq(std.mem.eql(u8, sliceByGraphemes(s, 6, 100), "!"), true);

    // 边界防护：反转区间、空区间、起始越界、空串均安全返回空切片
    expect_eq(sliceByGraphemes(s, 3, 3).len, @as(usize, 0));
    expect_eq(sliceByGraphemes(s, 5, 2).len, @as(usize, 0));
    expect_eq(sliceByGraphemes(s, 50, 60).len, @as(usize, 0));
    expect_eq(sliceByGraphemes("", 0, 5).len, @as(usize, 0));
}

test("Malformed and truncated UTF-8 fallback resilience") {
    // 孤立续字节 0x80、截断 4 字节头 0xF0 0x9F、合法 'A'
    const bad: [4]u8 = [0x80, 0xF0, 0x9F, 'A'];
    const d0 = decodeUtf8Step(bad[0..4]);
    expect_eq(d0.cp, @as(u21, 0xFFFD));
    expect_eq(d0.len, @as(usize, 1));

    // 迭代器遇到非法字节安全推进，不死循环、不崩溃
    expect_eq(count(bad[0..4]), @as(usize, 4));
}
