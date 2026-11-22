// Origin: Pure-CP Stdlib Test Suite (AI Coding Agent Token-Aware Structural Code Pattern Matcher) | Created: 2026-11-22
// Deep Production Verification Suite for lib/text/code_pattern.cp
// Covers:
//   1. Multiline & comment-immune matching plus tokenizeCodeLite multi-char operator accuracy
//   2. Single/multi metavariable captures ($OBJ, $METHOD, $ARG) and $_ wildcard behavior
//   3. Cross-position metavariable equality constraint ($X = $X + 1 matches a=a+1, rejects a=b+1)
//   4. Multiple hits with exact byte offsets, 1-based start/end line numbers & capture slices
//   5. Empty inputs, comment-only inputs, undersized pattern/hit/capture buffers & truncation safety

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CodeTokenKind,
    CodeToken,
    MetaCapture,
    PatternMatchHit,
    tokenizeCodeLite,
    findCodePatternMatches,
} from "../../lib/text/index.cp";

fn testMultilineCommentImmuneMatchAndTokenizer()! {
    // Verify multi-char operators and token kinds
    const op_src = "std::foo(x) -> bool { return x >= 10 && x != 42; }";
    let toks_buf: [32]CodeToken = [CodeToken{}; 32];
    let toks: []CodeToken = toks_buf[0..];
    const tok_count = tokenizeCodeLite(op_src, toks);
    expect_eq(tok_count, 18);
    expect_eq(toks[0].kind, CodeTokenKind.Ident);
    expect_eq(std.mem.eql(u8, toks[1].slice(op_src), "::"), true);
    expect_eq(std.mem.eql(u8, toks[6].slice(op_src), "->"), true);
    expect_eq(std.mem.eql(u8, toks[11].slice(op_src), ">="), true);
    expect_eq(toks[12].kind, CodeTokenKind.Number);
    expect_eq(std.mem.eql(u8, toks[13].slice(op_src), "&&"), true);
    expect_eq(std.mem.eql(u8, toks[15].slice(op_src), "!="), true);

    // Match pattern across newlines and inline/block comments
    const src =
        "fn check(status: i32) {\n" ++             // L1
        "    if (\n" ++                             // L2
        "        // ignore this comment\n" ++       // L3
        "        status /* block */ !=\n" ++        // L4
        "        0\n" ++                            // L5
        "    ) {\n" ++                              // L6
        "        return;\n" ++                      // L7
        "    }\n" ++                                // L8
        "}\n";                                      // L9

    let src_buf: [64]CodeToken = [CodeToken{}; 64];
    let pat_buf: [16]CodeToken = [CodeToken{}; 16];
    let hits_buf: [4]PatternMatchHit = [PatternMatchHit{}; 4];
    let caps_buf: [8]MetaCapture = [MetaCapture{}; 8];

    const n = findCodePatternMatches(
        src,
        "if ( $VAR != 0 )",
        src_buf[0..],
        pat_buf[0..],
        hits_buf[0..],
        caps_buf[0..],
    );
    expect_eq(n, 1);
    expect_eq(hits_buf[0].start_line, 2);
    expect_eq(hits_buf[0].end_line, 6);
    expect_eq(hits_buf[0].capture_count, 1);
    expect_eq(std.mem.eql(u8, caps_buf[0].var_name, "$VAR"), true);
    expect_eq(std.mem.eql(u8, caps_buf[0].captured_text, "status"), true);
}

fn testMultiMetaVarCapturesAndWildcard()! {
    const src =
        "const r1 = db.query(\"SELECT 1\", 100);\n" ++
        "const r2 = cache.put(key, \"val\");\n";

    let src_buf: [64]CodeToken = [CodeToken{}; 64];
    let pat_buf: [16]CodeToken = [CodeToken{}; 16];
    let hits_buf: [4]PatternMatchHit = [PatternMatchHit{}; 4];
    let caps_buf: [16]MetaCapture = [MetaCapture{}; 16];

    const n = findCodePatternMatches(
        src,
        "$OBJ.$METHOD($FIRST, $_)",
        src_buf[0..],
        pat_buf[0..],
        hits_buf[0..],
        caps_buf[0..],
    );
    expect_eq(n, 2);

    // Hit 0: db.query("SELECT 1", 100)
    expect_eq(hits_buf[0].capture_count, 3);
    const c0 = hits_buf[0].capture_start;
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 0].var_name, "$OBJ"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 0].captured_text, "db"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 1].var_name, "$METHOD"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 1].captured_text, "query"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 2].var_name, "$FIRST"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c0 + 2].captured_text, "\"SELECT 1\""), true);

    // Hit 1: cache.put(key, "val")
    expect_eq(hits_buf[1].capture_count, 3);
    const c1 = hits_buf[1].capture_start;
    expect_eq(std.mem.eql(u8, caps_buf[c1 + 0].captured_text, "cache"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c1 + 1].captured_text, "put"), true);
    expect_eq(std.mem.eql(u8, caps_buf[c1 + 2].captured_text, "key"), true);
}

fn testSameMetaVarConsistencyConstraint()! {
    const src =
        "a = b + 1;\n" ++             // L1: mismatch (a != b) -> MUST NOT match
        "counter = counter + 1;\n" ++ // L2: match ($X == counter)
        "x = y + 1;\n" ++             // L3: mismatch (x != y) -> MUST NOT match
        "idx = /* cmt */ idx + 1;\n"; // L4: match ($X == idx)

    let src_buf: [64]CodeToken = [CodeToken{}; 64];
    let pat_buf: [16]CodeToken = [CodeToken{}; 16];
    let hits_buf: [4]PatternMatchHit = [PatternMatchHit{}; 4];
    let caps_buf: [8]MetaCapture = [MetaCapture{}; 8];

    const n = findCodePatternMatches(
        src,
        "$X = $X + 1",
        src_buf[0..],
        pat_buf[0..],
        hits_buf[0..],
        caps_buf[0..],
    );
    expect_eq(n, 2);

    expect_eq(hits_buf[0].start_line, 2);
    expect_eq(hits_buf[0].end_line, 2);
    expect_eq(hits_buf[0].capture_count, 1);
    expect_eq(std.mem.eql(u8, caps_buf[hits_buf[0].capture_start].captured_text, "counter"), true);

    expect_eq(hits_buf[1].start_line, 4);
    expect_eq(hits_buf[1].end_line, 4);
    expect_eq(hits_buf[1].capture_count, 1);
    expect_eq(std.mem.eql(u8, caps_buf[hits_buf[1].capture_start].captured_text, "idx"), true);
}

fn testMultipleHitsAndExactSpans()! {
    const src =
        "// header\n" ++                  // L1
        "return err(400);\n" ++           // L2
        "let ok = 1;\n" ++                // L3
        "if (bad) {\n" ++                 // L4
        "    return\n" ++                 // L5
        "        err(404);\n" ++          // L6
        "}\n" ++                          // L7
        "const z = 0;\n" ++               // L8
        "return err(500);\n";             // L9

    let src_buf: [64]CodeToken = [CodeToken{}; 64];
    let pat_buf: [16]CodeToken = [CodeToken{}; 16];
    let hits_buf: [4]PatternMatchHit = [PatternMatchHit{}; 4];
    let caps_buf: [8]MetaCapture = [MetaCapture{}; 8];

    const n = findCodePatternMatches(
        src,
        "return err($CODE);",
        src_buf[0..],
        pat_buf[0..],
        hits_buf[0..],
        caps_buf[0..],
    );
    expect_eq(n, 3);

    expect_eq(hits_buf[0].start_line, 2);
    expect_eq(hits_buf[0].end_line, 2);
    expect_eq(std.mem.eql(u8, src[hits_buf[0].start_byte..hits_buf[0].end_byte], "return err(400);"), true);
    expect_eq(std.mem.eql(u8, caps_buf[hits_buf[0].capture_start].captured_text, "400"), true);

    expect_eq(hits_buf[1].start_line, 5);
    expect_eq(hits_buf[1].end_line, 6);
    expect_eq(std.mem.eql(u8, caps_buf[hits_buf[1].capture_start].captured_text, "404"), true);

    expect_eq(hits_buf[2].start_line, 9);
    expect_eq(hits_buf[2].end_line, 9);
    expect_eq(std.mem.eql(u8, src[hits_buf[2].start_byte..hits_buf[2].end_byte], "return err(500);"), true);
    expect_eq(std.mem.eql(u8, caps_buf[hits_buf[2].capture_start].captured_text, "500"), true);
}

fn testEmptyInputsAndBufferTruncationSafety()! {
    let src_buf: [32]CodeToken = [CodeToken{}; 32];
    let pat_buf: [8]CodeToken = [CodeToken{}; 8];
    let hits_buf: [4]PatternMatchHit = [PatternMatchHit{}; 4];
    let caps_buf: [4]MetaCapture = [MetaCapture{}; 4];

    // 1. Empty src, empty pattern, comment-only src/pattern -> 0
    expect_eq(findCodePatternMatches("", "$X = 1", src_buf[0..], pat_buf[0..], hits_buf[0..], caps_buf[0..]), 0);
    expect_eq(findCodePatternMatches("a = 1", "", src_buf[0..], pat_buf[0..], hits_buf[0..], caps_buf[0..]), 0);
    expect_eq(findCodePatternMatches("// only comment", "$X", src_buf[0..], pat_buf[0..], hits_buf[0..], caps_buf[0..]), 0);
    expect_eq(findCodePatternMatches("a = 1", "/* empty pat */", src_buf[0..], pat_buf[0..], hits_buf[0..], caps_buf[0..]), 0);

    // 2. Undersized pattern buffer safely returns 0 instead of matching truncated pattern
    let tiny_pat_buf: [2]CodeToken = [CodeToken{}; 2];
    expect_eq(findCodePatternMatches("a = 1", "$X = 1", src_buf[0..], tiny_pat_buf[0..], hits_buf[0..], caps_buf[0..]), 0);

    // 3. Undersized out_hits (cap=1) and zero-length out_captures safely count all 3 hits
    const three = "x = 1; y = 1; z = 1;";
    let one_hit_buf: [1]PatternMatchHit = [PatternMatchHit{}; 1];
    let zero_cap_buf: [0]MetaCapture = [];
    let zero_caps: []MetaCapture = zero_cap_buf[0..];
    const total = findCodePatternMatches(
        three,
        "$V = 1;",
        src_buf[0..],
        pat_buf[0..],
        one_hit_buf[0..],
        zero_caps,
    );
    expect_eq(total, 3);
    expect_eq(one_hit_buf[0].start_line, 1);
    expect_eq(one_hit_buf[0].capture_count, 0);
}

test("text code_pattern: multiline and comment-immune match plus tokenizer operators", testMultilineCommentImmuneMatchAndTokenizer);
test("text code_pattern: multi metavariable captures and $_ wildcard", testMultiMetaVarCapturesAndWildcard);
test("text code_pattern: cross-position same-$VAR consistency constraint", testSameMetaVarConsistencyConstraint);
test("text code_pattern: multiple hits with exact byte offsets and line numbers", testMultipleHitsAndExactSpans);
test("text code_pattern: empty inputs and undersized buffer truncation safety", testEmptyInputsAndBufferTruncationSafety);
