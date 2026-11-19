// Source: /data/dev/bun/src/string/ & /data/dev/bun/src/unicode/ & /data/dev/bun/src/highway/ & /data/dev/bun/src/collections/
// ============================================================================
// cap/test/test_string_simd_sort.cp
// Unified Industrial-grade Audit Test Suite for cap:string, cap:simd, cap:sort
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SimdScanner,
    REPLACEMENT_CHAR,
    Utf8,
    Utf16,
    UnicodeWidth,
    Utf8Grapheme,
    CaseConverter,
    SmolStr,
    HashedString,
    PathString,
    StringBuilder,
    StringJoiner,
    StringEscape,
} from "cap:string";
import { Simd } from "cap:simd";
import { Sort, SortOf } from "cap:sort";

fn testCapStringSubsystem(): !void {
    // 1. SmolStr (15-byte inline SSO vs heap fallback)
    let smol_in = SmolStr.init("cap-sso-15bytes");
    expect_eq(smol_in.isInline(), true);
    expect_eq(smol_in.isInlined(), true);
    expect_eq(smol_in.len(), 15);
    expect_eq(smol_in.eql("cap-sso-15bytes"), true);
    smol_in.release();

    let smol_heap = SmolStr.init("cap-heap-allocated-longer-than-15-bytes");
    expect_eq(smol_heap.isInline(), false);
    expect_eq(smol_heap.len(), 39);
    expect_eq(smol_heap.eql("cap-heap-allocated-longer-than-15-bytes"), true);
    smol_heap.release();

    // 2. HashedString (FNV-1a 32-bit pre-hashed O(1) comparison)
    const hs1 = HashedString.init("x-forwarded-for");
    const hs2 = HashedString.init("x-forwarded-for");
    const hs3 = HashedString.init("content-encoding");
    expect_eq(hs1.hashVal() == hs2.hashVal(), true);
    expect_eq(hs1.eql(hs2), true);
    expect_eq(hs1.eql(hs3), false);

    // 3. Utf8Grapheme (RFC 3629 validation, codepoints & UAX #29 extended grapheme clusters)
    const emoji_zwj = "👨‍👩‍👧"; // 3 emoji codepoints + 2 ZWJ (U+200D) = 5 codepoints, 1 grapheme cluster
    expect_eq(Utf8Grapheme.isValidUtf8(emoji_zwj), true);
    expect_eq(Utf8Grapheme.countCodepoints(emoji_zwj), 5);
    expect_eq(Utf8Grapheme.countGraphemes(emoji_zwj), 1);

    const crlf_text = "a\r\nb"; // 'a', '\r\n', 'b' -> 3 graphemes
    expect_eq(Utf8Grapheme.countGraphemes(crlf_text), 3);

    // 4. Utf8 / Utf16 transcoding & UnicodeWidth
    const cjk_emoji = "你好🚀";
    expect_eq(Utf8.isValid(cjk_emoji), true);
    expect_eq(Utf16.utf16LengthFromUtf8(cjk_emoji), 4); // 2 BMP + 1 surrogate pair (2 u16)
    let u16_buf: [16]u16 = [0; 16];
    const u16_len = Utf16.encodeUtf8ToUtf16Le(cjk_emoji, u16_buf[0..16]);
    expect_eq(u16_len, 4);
    let u8_buf: [32]u8 = [0; 32];
    const u8_len = Utf16.decodeUtf16LeToUtf8(u16_buf[0..u16_len], u8_buf[0..32]);
    expect_eq(std.mem.eql(u8, u8_buf[0..u8_len], cjk_emoji), true);
    expect_eq(UnicodeWidth.visibleWidth("\x1b[31m你好\x1b[0m"), 4);

    // 5. CaseConverter (snake_case, kebab-case, camelCase, PascalCase, SCREAMING_SNAKE_CASE)
    let case_buf: [64]u8 = undefined;
    const n_snake = CaseConverter.toSnakeCase("HTMLParserEngine", &case_buf);
    expect_eq(std.mem.eql(u8, case_buf[0..n_snake], "html_parser_engine"), true);

    const n_kebab = CaseConverter.toKebabCase("userAccountId", &case_buf);
    expect_eq(std.mem.eql(u8, case_buf[0..n_kebab], "user-account-id"), true);

    const n_camel = CaseConverter.toCamelCase("max_retry_count", &case_buf);
    expect_eq(std.mem.eql(u8, case_buf[0..n_camel], "maxRetryCount"), true);

    const n_pascal = CaseConverter.toPascalCase("http-response-header", &case_buf);
    expect_eq(std.mem.eql(u8, case_buf[0..n_pascal], "HttpResponseHeader"), true);

    const n_scream = CaseConverter.toScreamingSnakeCase("defaultBufferSize", &case_buf);
    expect_eq(std.mem.eql(u8, case_buf[0..n_scream], "DEFAULT_BUFFER_SIZE"), true);

    // 6. StringBuilder, StringJoiner, PathString & StringEscape
    let sb = StringBuilder.init();
    sb.count("cap:");
    sb.count("string");
    sb.allocate();
    * = sb.append("cap:");
    * = sb.append("string");
    expect_eq(std.mem.eql(u8, sb.slice(), "cap:string"), true);
    sb.release();

    let joiner = StringJoiner.init();
    joiner.pushDelimited("alpha", ",");
    joiner.pushDelimited("beta", ",");
    joiner.pushDelimited("gamma", ",");
    expect_eq(std.mem.eql(u8, joiner.slice(), "alpha,beta,gamma"), true);
    joiner.release();

    let ps = PathString.init("/data/dev/cp/lib/string/index.cp");
    expect_eq(ps.isAbsolute(), true);
    expect_eq(std.mem.eql(u8, ps.basename(), "index.cp"), true);
    expect_eq(std.mem.eql(u8, ps.dirname(), "/data/dev/cp/lib/string"), true);
    expect_eq(ps.hasExtension(".cp"), true);

    let esc_buf: [64]u8 = undefined;
    const esc_len = StringEscape.escapeHtml("<b>\"A&B\"</b>", &esc_buf);
    expect_eq(std.mem.eql(u8, esc_buf[0..esc_len], "&lt;b&gt;&quot;A&amp;B&quot;&lt;/b&gt;"), true);
}

fn testCapSimdSubsystem(): !void {
    // 1. 32-byte / 16-byte SIMD byte search & multi-char search
    const text = "0123456789abcdef0123456789abcdef_target:value;end\n";
    expect_eq(Simd.isAscii(text), true);
    expect_eq(Simd.firstNonAscii(text) == null, true);
    expect_eq(Simd.indexOfByte(text, '_').?, 32);
    expect_eq(Simd.indexOfChar(text, ':').?, 39);
    expect_eq(Simd.indexOfAny2(text, ':', ';').?, 39);
    expect_eq(Simd.indexOfAny4(text, ';', ':', '\n', '_').?, 32);
    expect_eq(Simd.indexOfAnyChar(text, ":;").?, 39);
    expect_eq(Simd.indexOfNewlineOrNonAscii(text).?, 49);
    expect_eq(Simd.countByte(text, '0'), 2);

    // 2. SIMD whitespace skipping
    const padded = "   \t\r\n   \t  \n   hello_simd";
    const ws_end = Simd.skipWhitespace(padded);
    expect_eq(ws_end, 16);
    expect_eq(std.mem.eql(u8, padded[ws_end..], "hello_simd"), true);

    // 3. SIMD batch ASCII case conversion
    const mixed = "Hello_SIMD_Vector_Engine_1234567890!";
    let lower_buf: [64]u8 = undefined;
    let upper_buf: [64]u8 = undefined;
    const l_len = Simd.toLowerAscii(mixed, &lower_buf);
    const u_len = Simd.toUpperAscii(mixed, &upper_buf);
    expect_eq(std.mem.eql(u8, lower_buf[0..l_len], "hello_simd_vector_engine_1234567890!"), true);
    expect_eq(std.mem.eql(u8, upper_buf[0..u_len], "HELLO_SIMD_VECTOR_ENGINE_1234567890!"), true);

    // 4. SIMD numerical vector reductions & dot product
    let nums: [10]i32 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    expect_eq(Simd.reduceAddI32(nums[0..]), 55);
    expect_eq(Simd.reduceMinI32(nums[0..]), 1);
    expect_eq(Simd.reduceMaxI32(nums[0..]), 10);

    let va: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    let vb: [4]f32 = [2.0, 3.0, 4.0, 5.0];
    expect_eq(Simd.dotProductF32(va[0..], vb[0..]), 40.0);
}

fn cmpAscI32(a: i32, b: i32): bool {
    return a < b;
}

fn cmpDescI32(a: i32, b: i32): bool {
    return a > b;
}

fn testCapSortSubsystem(): !void {
    // 1. PDQSort ascending & descending
    let data: [8]i32 = [64, -10, 25, 0, 12, 99, 25, 3];
    Sort.asc(data[0..]);
    expect_eq(Sort.isSortedAsc(data[0..]), true);
    expect_eq(data[0], -10);
    expect_eq(data[7], 99);

    // 2. Binary search, lowerBound & upperBound on sorted slice
    // Sorted data: [-10, 0, 3, 12, 25, 25, 64, 99]
    expect_eq(Sort.binarySearch(data[0..], 12).?, 3);
    expect_eq(Sort.binarySearch(data[0..], 77) == null, true);
    expect_eq(Sort.lowerBound(data[0..], 25), 4);
    expect_eq(Sort.upperBound(data[0..], 25), 6);

    // 3. PDQSort descending
    Sort.desc(data[0..]);
    expect_eq(Sort.isSorted(data[0..], cmpDescI32), true);
    expect_eq(data[0], 99);
    expect_eq(data[7], -10);

    // 4. Stable BlockSort & HeapSort
    let s_arr: [6]i32 = [9, 1, 5, 3, 8, 2];
    Sort.stable(s_arr[0..], cmpAscI32);
    expect_eq(Sort.isSortedAsc(s_arr[0..]), true);

    let h_arr: [6]i32 = [50, 10, 40, 20, 60, 30];
    Sort.heapAsc(h_arr[0..]);
    expect_eq(Sort.isSortedAsc(h_arr[0..]), true);
    expect_eq(h_arr[0], 10);
    expect_eq(h_arr[5], 60);

    // 5. Extrema & argMin / argMax
    let ext_arr: [5]i32 = [42, -99, 17, 256, 0];
    expect_eq(Sort.minI32(ext_arr[0..]).?, -99);
    expect_eq(Sort.maxI32(ext_arr[0..]).?, 256);
    expect_eq(Sort.argMin(ext_arr[0..]).?, 1);
    expect_eq(Sort.argMax(ext_arr[0..]).?, 3);
}

fn main() {
    test("1. cap:string — SmolStr (SSO), HashedString, Utf8Grapheme (UAX #29), UTF-16, CaseConverter & Builders", () => {
        try testCapStringSubsystem();
    });
    test("2. cap:simd — 16/32-byte SIMD byte scanners, whitespace skipper, ASCII batch case & vector reductions", () => {
        try testCapSimdSubsystem();
    });
    test("3. cap:sort — PDQSort, BlockSort (stable), HeapSort, binarySearch/lowerBound/upperBound & extrema", () => {
        try testCapSortSubsystem();
    });
}
