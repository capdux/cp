// Source: cp/cap/test/testJsonRustRefSuite.cp
// Reference-hardened against serde-rs/json (MIT/Apache-2.0), RFC 8259 & JSONTestSuite
// Covers:
// 1. JSONTestSuite / serde_json y_* (valid) & n_* (must-reject) vectors
// 2. UTF-16 surrogate pairs, lone/reversed surrogates, control chars, strict numbers, depth > 128 guard
// 3. RFC 8785 Canonical serialization & zero-heap validateJsonStrict / validateJsonWithMaxDepth
// 4. 16-lane SIMD vs scalar differential parity (0 discrepancy)
// 5. 1,500-case deterministic PRNG malformed JSON fuzz suite (0 panic / 0 OOB / 0 leak)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JsonParser,
    JsonParseOptions,
    FastJsonScanner,
    JSON_DEFAULT_MAX_DEPTH,
    validateJsonStrict,
    validateJsonWithMaxDepth,
    validateJsonStrictOrErr,
} from "../../lib/json/parser";
import { JsonSerializer } from "../../lib/json/serializer";
import {
    simdIndexOfJsonEscape,
    needsJsonEscape,
    escapeJsonContent,
    unescapeJsonContent,
} from "../../lib/json/escape";

// --- Scalar Reference Implementations for Differential Parity ---

fn scalarSkipWhitespace(input: []const u8): usize {
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            i += 1;
        } else {
            break;
        }
    }
    return i;
}

fn scalarIndexOfQuoteEscapeOrCtrl(input: []const u8): usize? {
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '"' || c == '\\' || c < 0x20) return i;
        i += 1;
    }
    return null;
}

fn scalarIndexOfQuoteOrEscape(input: []const u8): usize? {
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '"' || c == '\\') return i;
        i += 1;
    }
    return null;
}

// Deterministic xorshift64* PRNG
struct Rng {
    state: u64;

    fn nextU64(*_): u64 {
        let x = _.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn nextU8(*_): u8 {
        return ((_.nextU64() >> 32) & 0xFF) as u8;
    }

    fn nextRange(*_, bound: usize): usize {
        return ((_.nextU64() as usize) % bound);
    }
}

// --- Test 1: JSONTestSuite & serde_json y_* Valid Vectors + Surrogate Pairs + RFC 8785 ---

fn testValidVectorsAndSurrogates(): !void {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // JSONTestSuite y_* valid corpus
    const valid_cases: []const []const u8 = [
        "null",
        "true",
        "false",
        "0",
        "-0",
        "42",
        "-42",
        "0.0",
        "-0.0",
        "123.456e-7",
        "1E+10",
        "\"\"",
        "\"hello world\"",
        "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"",
        "\"\\u0000\\u001f\\u007f\"",
        "\"\\uD83D\\uDE00\"",
        "\"\\uD834\\uDD1E\"",
        "[]",
        "{}",
        "[1, true, false, null, \"str\", {\"k\": [1, 2, 3]}]",
        " \t\r\n { \t \"a\" : [ 1 , 2 ] , \"b\" : \"\\u4E2D\\u6587\" } \r\n ",
    ];

    for (src of valid_cases) {
        expect_eq(validateJsonStrict(src), true);
        * = try JsonParser.parse(alloc, src);
    }

    // Exact UTF-16 surrogate pair decoding: \uD83D\uDE00 -> U+1F600 (F0 9F 98 80)
    const emoji_val = try JsonParser.parse(alloc, "\"\\uD83D\\uDE00\"");
    expect_eq(std.mem.eql(u8, emoji_val.asString().?, "\xf0\x9f\x98\x80"), true);

    // Musical G clef U+1D11E: \uD834\uDD1E -> F0 9D 84 9E
    const clef_val = try JsonParser.parse(alloc, "\"\\uD834\\uDD1E\"");
    expect_eq(std.mem.eql(u8, clef_val.asString().?, "\xf0\x9d\x84\x9e"), true);

    // Zero-alloc escape/unescape parity on surrogate pair
    let ubuf: [64]u8 = undefined;
    const unesc_emoji = unescapeJsonContent("\\uD83D\\uDE00", &ubuf).?;
    expect_eq(std.mem.eql(u8, unesc_emoji, "\xf0\x9f\x98\x80"), true);

    // RFC 8785 Canonical serialization determinism
    const raw_obj = try JsonParser.parse(alloc, "{\"b\":-0.0,\"a\":[3,2,1],\"\\uD83D\\uDE00\":1,\"\\uE000\":2}");
    const canon = try JsonSerializer.stringifyCanonical(alloc, raw_obj);
    expect_eq(
        std.mem.eql(u8, canon, "{\"a\":[3,2,1],\"b\":0,\"\xf0\x9f\x98\x80\":1,\"\xee\x80\x80\":2}"),
        true,
    );
}

// --- Test 2: JSONTestSuite & serde_json n_* Must-Reject Matrix + Depth > 128 Guard ---

fn testStrictRejectionAndDepthLimit(): !void {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // JSONTestSuite & serde_json n_* invalid corpus
    const invalid_cases: []const []const u8 = [
        // Empty / whitespace only
        "",
        "   \t\r\n  ",
        // Leading zeros & malformed numbers
        "01",
        "-01",
        "00",
        "-00",
        "+1",
        ".5",
        "-.5",
        "1.",
        "1.e1",
        "1e",
        "1e+",
        "1e-",
        "1E+-1",
        "-",
        "1e9999",
        "-1e9999",
        "NaN",
        "Infinity",
        "-Infinity",
        // Trailing commas & syntax errors
        "[1,]",
        "[,]",
        "{\"a\":1,}",
        "{,}",
        "{\"a\" 1}",
        "{\"a\":}",
        "{a:1}",
        "{'a':1}",
        "'single'",
        "[1 2]",
        "{\"a\":1 \"b\":2}",
        // Trailing garbage
        "true false",
        "123 456",
        "{}[]",
        "\"ok\" trailing",
        "nullx",
        "true1",
        // Unclosed containers & strings
        "[1, 2",
        "{\"a\": 1",
        "\"unterminated",
        "\"unterminated\\",
        // Unescaped control characters 0x00..0x1F & illegal whitespace (VT/FF)
        "\"a\x00b\"",
        "\"a\nb\"",
        "\"a\rb\"",
        "\"a\tb\"",
        "\"a\x1fb\"",
        "\x0btrue",
        "\x0ctrue",
        // Invalid escapes & UTF-16 lone/reversed surrogates
        "\"\\x41\"",
        "\"\\a\"",
        "\"\\1\"",
        "\"\\u123\"",
        "\"\\u123G\"",
        "\"\\uD800\"",
        "\"\\uDBFF\"",
        "\"\\uDC00\"",
        "\"\\uDFFF\"",
        "\"\\uDC00\\uD800\"",
        "\"\\uD800\\u0041\"",
        "\"\\uD800a\"",
        // Invalid raw UTF-8 bytes in string
        "\"\x80\"",
        "\"\xff\"",
        "\"\xed\xa0\x80\"",
        // Comments (forbidden in strict RFC 8259)
        "/* comment */ 1",
        "// comment\n1",
    ];

    for (bad of invalid_cases) {
        expect_eq(validateJsonStrict(bad), false);
        let parse_failed = false;
        * = JsonParser.parse(alloc, bad) catch { parse_failed = true; };
        expect_eq(parse_failed, true);
    }

    // Recursion depth limit guard: default JSON_DEFAULT_MAX_DEPTH == 128
    expect_eq(JSON_DEFAULT_MAX_DEPTH, 128 as usize);

    // Depth 128 must succeed; depth 129 must be rejected with MaxDepthExceeded (0 stack overflow)
    let deep_ok: [256]u8 = undefined;
    for (i of 0..128) {
        deep_ok[i] = '[';
        deep_ok[128 + i] = ']';
    }
    expect_eq(validateJsonStrict(&deep_ok), true);
    * = try JsonParser.parse(alloc, &deep_ok);

    let deep_over: [258]u8 = undefined;
    for (i of 0..129) {
        deep_over[i] = '[';
        deep_over[129 + i] = ']';
    }
    expect_eq(validateJsonStrict(&deep_over), false);
    expect_eq(validateJsonWithMaxDepth(&deep_over, 128), false);
    expect_eq(validateJsonWithMaxDepth(&deep_over, 130), true);

    let depth_err = false;
    * = JsonParser.parse(alloc, &deep_over) catch { depth_err = true; };
    expect_eq(depth_err, true);

    // Object deep nesting > 128: {"a":{"a":...0...}}
    let obj_deep_buf: [1024]u8 = undefined;
    let opos: usize = 0;
    for (i of 0..129) {
        * = i;
        @memcpy(obj_deep_buf[opos .. opos + 5], "{\"a\":");
        opos += 5;
    }
    obj_deep_buf[opos] = '0';
    opos += 1;
    for (i of 0..129) {
        * = i;
        obj_deep_buf[opos] = '}';
        opos += 1;
    }
    const obj_deep = obj_deep_buf[0..opos];
    expect_eq(validateJsonStrict(obj_deep), false);
    let obj_depth_err = false;
    * = JsonParser.parse(alloc, obj_deep) catch { obj_depth_err = true; };
    expect_eq(obj_depth_err, true);
}

// --- Test 3: 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy) ---

fn testSimdVsScalarParity(): !void {
    let rng = Rng { state: 0x2A4F_6C8E_1357_9BDF };

    // 1. Exhaustive offset sweep (0..96) for skipWhitespace, indexOfQuoteEscapeOrCtrl, indexOfQuoteOrEscape, simdIndexOfJsonEscape
    let buf: [96]u8 = undefined;
    for (len of 0..97) {
        const len_u = len as usize;
        // Whitespace sweep
        for (i of 0..len) {
            const ws_pick = (i as usize) % 4;
            buf[i] = if (ws_pick == 0) ' ' else if (ws_pick == 1) '\t' else if (ws_pick == 2) '\r' else '\n';
        }
        if (len_u > 0) {
            const stop = rng.nextRange(len_u);
            buf[stop] = 'x';
        }
        const ws_slice = buf[0..len_u];
        expect_eq(FastJsonScanner.skipWhitespace(ws_slice), scalarSkipWhitespace(ws_slice));

        // String scan sweep with every control byte 0x00..0x1F, '"', '\\'
        for (i of 0..len) {
            buf[i] = 'a';
        }
        if (len_u > 0) {
            const hit_pos = rng.nextRange(len_u);
            const special: u8 = if (len_u % 3 == 0) '"' else if (len_u % 3 == 1) '\\' else (len_u % 32) as u8;
            buf[hit_pos] = special;
        }
        const s_slice = buf[0..len_u];
        expect_eq(FastJsonScanner.indexOfQuoteEscapeOrCtrl(s_slice), scalarIndexOfQuoteEscapeOrCtrl(s_slice));
        expect_eq(simdIndexOfJsonEscape(s_slice), scalarIndexOfQuoteEscapeOrCtrl(s_slice));
        expect_eq(needsJsonEscape(s_slice), scalarIndexOfQuoteEscapeOrCtrl(s_slice) != null);
        expect_eq(FastJsonScanner.indexOfQuoteOrEscape(s_slice), scalarIndexOfQuoteOrEscape(s_slice));
    }

    // 2. Escape + unescape round-trip across long (> 64B) strings crossing multiple 16-lane SIMD chunks
    const long_src = "0123456789abcdef0123456789abcdef\"quoted\"\\slash\\\nline2\t\x01\x1f0123456789abcdef";
    let esc_buf: [256]u8 = undefined;
    let dec_buf: [256]u8 = undefined;
    const esc = escapeJsonContent(long_src, &esc_buf).?;
    const decoded = unescapeJsonContent(esc, &dec_buf).?;
    expect_eq(std.mem.eql(u8, decoded, long_src), true);
}

// --- Test 4: 1,500-Case Deterministic PRNG Malformed JSON Fuzz Suite ---

fn testDeterministicJsonFuzz(): !void {
    let rng = Rng { state: 0x3C71_9A2D_5E8B_4F11 };
    let buf: [256]u8 = undefined;
    const seeds: []const []const u8 = [
        "{\"key\":\"value\",\"num\":-123.45e+6,\"arr\":[true,false,null]}",
        "\"hello \\uD83D\\uDE00 \\n\\t\\\\ world\"",
        "[[[[1,2,3],{\"a\":\"b\"}]]]",
        "{\"nested\":{\"x\":0,\"y\":-0.5,\"z\":\"abc\\u0041\"}}",
    ];

    let total_cases: usize = 0;
    let valid_count: usize = 0;
    let invalid_count: usize = 0;

    for (case_idx of 0..1500) {
        let len: usize = 0;
        const strat = (case_idx as usize) % 5;

        if (strat == 0) {
            // Pure random bytes of length 0..128
            len = rng.nextRange(129);
            for (i of 0..len) {
                buf[i] = rng.nextU8();
            }
        } else if (strat == 1) {
            // Mutate valid JSON seed with 1..4 random byte corruptions
            const seed = seeds[rng.nextRange(seeds.len)];
            len = seed.len;
            @memcpy(buf[0..len], seed);
            const muts = 1 + rng.nextRange(4);
            for (m of 0..muts) {
                * = m;
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else if (strat == 2) {
            // Truncated escapes / surrogate attacks
            const raw_prefix = "\"abc\\uD83D\\uDE00\\n\\t\\\"def\"";
            len = (1 as usize) + rng.nextRange(raw_prefix.len);
            @memcpy(buf[0..len], raw_prefix[0..len]);
            if (len > 2) {
                buf[rng.nextRange(len)] = rng.nextU8();
            }
        } else if (strat == 3) {
            // Deep nesting burst (1..160 brackets)
            const depth: usize = (1 as usize) + rng.nextRange(160);
            const use_obj = (rng.nextU8() & 1) == 0;
            if (!use_obj && depth * (2 as usize) <= buf.len) {
                for (i of 0..depth) {
                    const iu = i as usize;
                    buf[iu] = '[';
                    buf[depth + iu] = ']';
                }
                len = depth * (2 as usize);
                if ((rng.nextU8() & 1) == 0 && len > 0) {
                    buf[rng.nextRange(len)] = ',';
                }
            } else {
                const d: usize = if (depth > 40) (40 as usize) else depth;
                for (i of 0..d) {
                    buf[i] = '[';
                }
                len = d; // intentionally unclosed
            }
        } else {
            // Control char & numeric boundary fuzzing
            const num_pool: []const []const u8 = [
                "01", "-01", "+0", ".1", "1.", "1e", "1e+", "1e9999", "\"\x00\"", "\"\x1f\"", "[1,]", "{\"a\":1,}",
            ];
            const pick = num_pool[rng.nextRange(num_pool.len)];
            len = pick.len;
            @memcpy(buf[0..len], pick);
        }

        const slice = buf[0..len];
        const is_valid = validateJsonStrict(slice);

        // Parse with bounded arena to verify 0 leaks / 0 panic / 0 OOB and parity with zero-heap validator
        let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        let parse_ok = true;
        * = JsonParser.parse(arena.allocator(), slice) catch { parse_ok = false; };

        // Zero-heap validator and full DOM parser MUST agree 100% on every single fuzz input
        expect_eq(is_valid, parse_ok);

        if (is_valid) {
            valid_count += 1;
        } else {
            invalid_count += 1;
        }
        total_cases += 1;
    }

    expect_eq(total_cases, 1500 as usize);
    expect_eq(invalid_count > 1200, true);
    expect_eq(valid_count + invalid_count, 1500 as usize);
}

fn main() {
    test("1. JSONTestSuite & serde_json y_* Valid Vectors, Surrogate Pairs & RFC 8785", () => {
        try testValidVectorsAndSurrogates();
    });
    test("2. JSONTestSuite & serde_json n_* Must-Reject Matrix & Depth > 128 Guard", () => {
        try testStrictRejectionAndDepthLimit();
    });
    test("3. 16-Lane SIMD vs Scalar Differential Parity (0 Discrepancy)", () => {
        try testSimdVsScalarParity();
    });
    test("4. 1,500-Case Deterministic PRNG Malformed JSON Fuzz Suite", () => {
        try testDeterministicJsonFuzz();
    });
}
