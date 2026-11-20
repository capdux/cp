// Source: cp/cap/test/test_unicode_production_deep.cp
// Deep Production Verification Suite for cap:unicode & cap.unicode

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    UnicodeError,
    isValidUtf8,
    countCodepoints,
    decodeUtf8Step,
    encodeUtf8,
    utf8ToUtf16,
    utf16ToUtf8,
    isWhitespace,
    isControl,
    isAscii,
    isIdentifierStart,
    isIdentifierContinue,
    toLowerCodepoint,
    toUpperCodepoint,
    toLowerUtf8,
    toUpperUtf8,
    GraphemeIterator,
    countGraphemes,
    codepointWidth,
    displayWidth,
    truncateByWidth,
} from "cap:unicode";

test("cap:unicode - UTF-8/UTF-16 codecs, surrogate pairs & UnicodeError guards") {
    const alloc = std.testing.allocator;

    // 1. Valid UTF-8 & codepoint counting
    const sample = "A中😀";
    expect_eq(isValidUtf8(sample), true);
    expect_eq(countCodepoints(sample), 3);

    // 2. Invalid UTF-8 sequences (overlong, surrogate, truncated, invalid continuation)
    expect_eq(isValidUtf8("\xC0\xAF"), false);
    expect_eq(isValidUtf8("\xED\xA0\x80"), false);
    expect_eq(isValidUtf8("\xF0\x9F\x98"), false);
    expect_eq(isValidUtf8("\x80"), false);

    let err_decode = false;
    * = decodeUtf8Step("\xED\xA0\x80", 0) catch |e| {
        err_decode = (e == UnicodeError.InvalidUtf8);
    };
    expect_eq(err_decode, true);

    // 3. encodeUtf8 & error guards
    let enc_buf: [4]u8 = [0, 0, 0, 0];
    const n_emoji = encodeUtf8(0x1F600, enc_buf[0..]) catch unreachable;
    expect_eq(n_emoji, 4);
    expect_eq(std.mem.eql(u8, enc_buf[0..4], "😀"), true);

    let err_cp = false;
    * = encodeUtf8(0xD800, enc_buf[0..]) catch |e| {
        err_cp = (e == UnicodeError.InvalidCodepoint);
    };
    expect_eq(err_cp, true);

    let small_buf: [2]u8 = [0, 0];
    let err_small = false;
    * = encodeUtf8(0x4E2D, small_buf[0..]) catch |e| {
        err_small = (e == UnicodeError.BufferTooSmall);
    };
    expect_eq(err_small, true);

    // 4. UTF-8 <-> UTF-16 surrogate pair round-trip
    const u16_slice = utf8ToUtf16(alloc, sample) catch unreachable;
    defer alloc.free(u16_slice);
    expect_eq(u16_slice.len, 4); // 'A' (1) + '中' (1) + '😀' (2 surrogate units)
    expect_eq(u16_slice[0], 0x0041);
    expect_eq(u16_slice[1], 0x4E2D);
    expect_eq(u16_slice[2], 0xD83D);
    expect_eq(u16_slice[3], 0xDE00);

    const u8_roundtrip = utf16ToUtf8(alloc, u16_slice) catch unreachable;
    defer alloc.free(u8_roundtrip);
    expect_eq(std.mem.eql(u8, u8_roundtrip, sample), true);

    // 5. Unpaired surrogate interception
    const bad_high: [1]u16 = [0xD83D];
    let err_high = false;
    * = utf16ToUtf8(alloc, bad_high[0..]) catch |e| {
        err_high = (e == UnicodeError.UnpairedSurrogate);
    };
    expect_eq(err_high, true);

    const bad_low: [2]u16 = [0x0041, 0xDC00];
    let err_low = false;
    * = utf16ToUtf8(alloc, bad_low[0..]) catch |e| {
        err_low = (e == UnicodeError.UnpairedSurrogate);
    };
    expect_eq(err_low, true);
}

test("cap:unicode - UAX #29 Grapheme clusters (ZWJ, flags, CRLF, combining, Hangul, skin tone)") {
    // 1. CRLF must not split
    expect_eq(countGraphemes("a\r\nb"), 3);

    // 2. Combining acute accent: "e\u{0301}" -> 1 cluster
    expect_eq(countGraphemes("e\u{0301}x"), 2);

    // 3. ZWJ Emoji family & skin-tone modifier
    expect_eq(countGraphemes("👨‍👩‍👧‍👦"), 1);
    expect_eq(countGraphemes("👩🏻‍💻"), 1);

    // 4. Regional Indicator flag pairs: two flags -> 2 clusters
    expect_eq(countGraphemes("🇨🇳🇺🇸"), 2);

    // 5. Hangul Jamo L + V + T -> 1 cluster
    expect_eq(countGraphemes("\u{1100}\u{1161}\u{11A8}"), 1);

    // 6. GraphemeIterator peek / next / reset
    let it = GraphemeIterator.init("e\u{0301}🇨🇳!");
    const p1 = it.peek().?;
    expect_eq(std.mem.eql(u8, p1, "e\u{0301}"), true);
    const g1 = it.next().?;
    expect_eq(std.mem.eql(u8, g1, "e\u{0301}"), true);
    const g2 = it.next().?;
    expect_eq(std.mem.eql(u8, g2, "🇨🇳"), true);
    const g3 = it.next().?;
    expect_eq(std.mem.eql(u8, g3, "!"), true);
    expect_eq(it.next() == null, true);

    it.reset();
    expect_eq(std.mem.eql(u8, it.next().?, "e\u{0301}"), true);
}

test("cap:unicode - displayWidth, codepointWidth & truncateByWidth (CJK, Emoji, ANSI)") {
    const alloc = std.testing.allocator;

    expect_eq(codepointWidth('A'), 1);
    expect_eq(codepointWidth(0x4E2D), 2); // '中'
    expect_eq(codepointWidth(0x0301), 0); // combining mark
    expect_eq(codepointWidth(0x000A), 0); // control char
    expect_eq(codepointWidth(0x1F600), 2); // '😀'

    expect_eq(displayWidth("Hello世界"), 9);
    expect_eq(displayWidth("e\u{0301}👨‍👩‍👧‍👦🇺🇸"), 5); // 1 + 2 + 2
    expect_eq(displayWidth("\x1b[31mRed\x1b[0m中"), 5); // ANSI skipped

    // Truncate CJK + Emoji without splitting clusters
    const mixed = "Hi世界😀!";
    // Widths: H(1) i(1) 世(2) 界(2) 😀(2) !(1) => total 9
    const t1 = truncateByWidth(alloc, mixed, 6, "...") catch unreachable;
    defer alloc.free(t1);
    // budget = 6 - 3 = 3 => "Hi" (width 2), next "世" would make 4 > 3 => "Hi..."
    expect_eq(std.mem.eql(u8, t1, "Hi..."), true);

    const t2 = truncateByWidth(alloc, mixed, 7, "...") catch unreachable;
    defer alloc.free(t2);
    // budget = 7 - 3 = 4 => "Hi世..."
    expect_eq(std.mem.eql(u8, t2, "Hi世..."), true);

    const t_fit = truncateByWidth(alloc, mixed, 9, "...") catch unreachable;
    defer alloc.free(t_fit);
    expect_eq(std.mem.eql(u8, t_fit, mixed), true);
}

test("cap:unicode - Unicode properties, identifiers & case mapping") {
    const alloc = std.testing.allocator;

    expect_eq(isAscii('Z'), true);
    expect_eq(isAscii(0x00E9), false);
    expect_eq(isControl('\n'), true);
    expect_eq(isControl(0x0085), true);
    expect_eq(isControl(' '), false);

    expect_eq(isWhitespace(' '), true);
    expect_eq(isWhitespace(0x00A0), true); // NBSP
    expect_eq(isWhitespace(0x3000), true); // Ideographic space
    expect_eq(isWhitespace('a'), false);

    expect_eq(isIdentifierStart('_'), true);
    expect_eq(isIdentifierStart(0x4E2D), true); // '中'
    expect_eq(isIdentifierStart('0'), false);
    expect_eq(isIdentifierContinue('0'), true);
    expect_eq(isIdentifierContinue(0x0301), true);

    expect_eq(toLowerCodepoint('A'), 'a');
    expect_eq(toLowerCodepoint(0x00C9), 0x00E9); // 'É' -> 'é'
    expect_eq(toUpperCodepoint(0x00E9), 0x00C9); // 'é' -> 'É'
    expect_eq(toLowerCodepoint(0x03A9), 0x03C9); // 'Ω' -> 'ω'
    expect_eq(toUpperCodepoint(0x0430), 0x0410); // 'а' -> 'А'

    const lower = toLowerUtf8(alloc, "Hello ÉΩА") catch unreachable;
    defer alloc.free(lower);
    expect_eq(std.mem.eql(u8, lower, "hello éωа"), true);

    const upper = toUpperUtf8(alloc, "Hello éωа") catch unreachable;
    defer alloc.free(upper);
    expect_eq(std.mem.eql(u8, upper, "HELLO ÉΩА"), true);
}

test("cap:unicode - cap.unicode dynamic mounting & zero leaks") {
    const alloc = std.testing.allocator;

    expect_eq(cap.unicode.isValidUtf8("CP-Unicode"), true);
    expect_eq(cap.unicode.countGraphemes("👨‍👩‍👧‍👦"), 1);
    expect_eq(cap.unicode.displayWidth("中文"), 4);

    const cut = cap.unicode.truncateByWidth(alloc, "中文测试", 5, "..") catch unreachable;
    defer alloc.free(cut);
    expect_eq(std.mem.eql(u8, cut, "中.."), true);
}
