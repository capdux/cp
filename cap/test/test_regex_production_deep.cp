// Source: cp/cap/test/test_regex_production_deep.cp
// Deep Production Test Suite for cap:regex & cap.regex
// Covers:
//   1. Basic syntax, char classes [a-z0-9_-]+ / [^...], escapes \d\w\s, anchors ^...$, alternation |
//   2. Capture groups (\d{4})-(\d{2})-(\d{2}), non-capturing (?:...), lazy ".*?" vs greedy ".*"
//   3. findAll, replaceAll (with $1/$2/$0 group substitution), and split
//   4. Anti-ReDoS linear-time guarantee ((a+)+b on "a"*80 + "c") & malformed pattern rejection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.regex`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Regex, RegexError, MatchSpan, Captures } from "cap:compiler";
import cap from "cap";

fn strEq(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn expectCompileError(alloc: std.mem.Allocator, pattern: string, expected_err: RegexError): ! {
    let re = Regex.compile(alloc, pattern) catch |e| {
        expect_eq(e == expected_err, true);
        return;
    };
    re.deinit();
    return error.ExpectedCompileError;
}

fn testBasicSyntaxClassesEscapesAnchors(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Char class [a-z0-9_-]+ with ^...$ anchors
    let slug_re = Regex.compile(alloc, "^[a-z0-9_-]+$")?;
    defer slug_re.deinit();
    expect_eq(slug_re.isMatch("cp-lang_2026"), true);
    expect_eq(slug_re.isMatch("Invalid!Slug"), false);
    expect_eq(slug_re.isFullMatch("valid_id-01"), true);

    // 2. Negated char class [^0-9]+
    let non_digit = Regex.compile(alloc, "^[^0-9]+$")?;
    defer non_digit.deinit();
    expect_eq(non_digit.isMatch("hello-world"), true);
    expect_eq(non_digit.isMatch("hello123"), false);

    // 3. Escapes \d \w \s and literal \.
    let log_re = Regex.compile(alloc, "^\\w+\\s+\\d+\\.\\d+$")?;
    defer log_re.deinit();
    expect_eq(log_re.isMatch("latency 12.34"), true);
    expect_eq(log_re.isMatch("latency 12x34"), false);

    // 4. Alternation |
    let route_re = Regex.compile(alloc, "^/(api|v1|health)/status$")?;
    defer route_re.deinit();
    expect_eq(route_re.isMatch("/api/status"), true);
    expect_eq(route_re.isMatch("/v1/status"), true);
    expect_eq(route_re.isMatch("/health/status"), true);
    expect_eq(route_re.isMatch("/v2/status"), false);
}

fn testCapturesNonCapturingAndLazyVsGreedy(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Capture groups (\d{4})-(\d{2})-(\d{2})
    let date_re = Regex.compile(alloc, "(\\d{4})-(\\d{2})-(\\d{2})")?;
    defer date_re.deinit();

    let caps_opt = date_re.captures(alloc, "released on 2026-11-20 in prod")?;
    expect_eq(caps_opt != null, true);
    let caps = caps_opt.?;
    defer caps.deinit();

    expect_eq(caps.count(), 4);
    expect_eq(strEq(caps.group(0).?.text, "2026-11-20"), true);
    expect_eq(strEq(caps.group(1).?.text, "2026"), true);
    expect_eq(strEq(caps.group(2).?.text, "11"), true);
    expect_eq(strEq(caps.group(3).?.text, "20"), true);

    // 2. Non-capturing group (?:...) + capturing group (...)
    let proto_re = Regex.compile(alloc, "(?:http|https)://([a-z.-]+):(\\d{2,5})")?;
    defer proto_re.deinit();
    let p_caps_opt = proto_re.captures(alloc, "https://cp-lang.org:8443/docs")?;
    expect_eq(p_caps_opt != null, true);
    let p_caps = p_caps_opt.?;
    defer p_caps.deinit();

    expect_eq(p_caps.count(), 3);
    expect_eq(strEq(p_caps.group(1).?.text, "cp-lang.org"), true);
    expect_eq(strEq(p_caps.group(2).?.text, "8443"), true);

    // 3. Lazy ".*?" vs Greedy ".*"
    const quoted_input = "a=\"first\" b=\"second\"";
    let lazy_re = Regex.compile(alloc, "\".*?\"")?;
    defer lazy_re.deinit();
    const lazy_hit = lazy_re.find(quoted_input);
    expect_eq(lazy_hit != null, true);
    expect_eq(strEq(lazy_hit.?.text, "\"first\""), true);

    let greedy_re = Regex.compile(alloc, "\".*\"")?;
    defer greedy_re.deinit();
    const greedy_hit = greedy_re.find(quoted_input);
    expect_eq(greedy_hit != null, true);
    expect_eq(strEq(greedy_hit.?.text, "\"first\" b=\"second\""), true);
}

fn testFindAllReplaceAllAndSplit(): ! {
    const alloc = std.heap.page_allocator;

    // 1. findAll
    let num_re = Regex.compile(alloc, "\\d+")?;
    defer num_re.deinit();
    const nums = num_re.findAll(alloc, "cpu=16, mem=64, disk=512")?;
    defer alloc.free(nums);
    expect_eq(nums.len, 3);
    expect_eq(strEq(nums[0].text, "16"), true);
    expect_eq(strEq(nums[1].text, "64"), true);
    expect_eq(strEq(nums[2].text, "512"), true);

    // 2. replaceAll with $1/$2 capture group reordering
    let kv_re = Regex.compile(alloc, "(\\w+)=(\\d+)")?;
    defer kv_re.deinit();
    const replaced = kv_re.replaceAll(alloc, "a=1 b=2", "[$2:$1]")?;
    defer alloc.free(replaced);
    expect_eq(strEq(replaced, "[1:a] [2:b]"), true);

    // 3. split
    let sep_re = Regex.compile(alloc, "\\s*,\\s*")?;
    defer sep_re.deinit();
    const parts = sep_re.split(alloc, "alpha, beta ,  gamma,delta")?;
    defer alloc.free(parts);
    expect_eq(parts.len, 4);
    expect_eq(strEq(parts[0], "alpha"), true);
    expect_eq(strEq(parts[1], "beta"), true);
    expect_eq(strEq(parts[2], "gamma"), true);
    expect_eq(strEq(parts[3], "delta"), true);
}

fn testAntiRedosAndMalformedPatterns(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Anti-ReDoS: (a+)+b against 80 'a's + 'c' must finish in linear O(N*M) time
    let redos_re = Regex.compile(alloc, "(a+)+b")?;
    defer redos_re.deinit();

    let evil_buf: [81]u8 = undefined;
    for (i of 0..80) {
        evil_buf[i] = 'a';
    }
    evil_buf[80] = 'c';
    expect_eq(redos_re.isMatch(evil_buf[0..81]), false);

    // Positive sanity check on same pattern
    evil_buf[80] = 'b';
    expect_eq(redos_re.isMatch(evil_buf[0..81]), true);

    // 2. Malformed pattern rejections
    expectCompileError(alloc, "(abc", RegexError.UnmatchedParen)?;
    expectCompileError(alloc, "abc)", RegexError.UnmatchedParen)?;
    expectCompileError(alloc, "[a-z", RegexError.UnclosedClass)?;
    expectCompileError(alloc, "*abc", RegexError.InvalidQuantifier)?;
    expectCompileError(alloc, "a{5,2}", RegexError.InvalidQuantifier)?;
    expectCompileError(alloc, "abc\\", RegexError.InvalidEscape)?;
}

fn testCapRegexDynamicMount(): ! {
    const alloc = std.heap.page_allocator;

    let re = cap.compiler.Regex.compile(alloc, "^cap_([a-z]+)_(\\d+)$")?;
    defer re.deinit();

    const m = re.find("cap_regex_01");
    expect_eq(m != null, true);
    expect_eq(strEq(m.?.text, "cap_regex_01"), true);

    const out = re.replaceAll(alloc, "cap_regex_01", "$1:$2")?;
    defer alloc.free(out);
    expect_eq(strEq(out, "regex:01"), true);
}

fn main() {
    test("1. Basic Syntax, Char Classes, Escapes, Anchors & Alternation", () => {
        testBasicSyntaxClassesEscapesAnchors()?;
    });

    test("2. Capture Groups, Non-Capturing (?:...), and Lazy .*? vs Greedy .*", () => {
        testCapturesNonCapturingAndLazyVsGreedy()?;
    });

    test("3. findAll, replaceAll ($1/$2 Capture Substitution) & split", () => {
        testFindAllReplaceAllAndSplit()?;
    });

    test("4. Anti-ReDoS O(N*M) Linear Time ((a+)+b vs a*80+c) & Malformed Pattern Rejection", () => {
        testAntiRedosAndMalformedPatterns()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.regex`", () => {
        testCapRegexDynamicMount()?;
    });
}
