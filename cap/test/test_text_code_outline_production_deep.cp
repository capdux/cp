// Origin: Pure-CP Stdlib Test Suite (AI Coding Agent Zero-Alloc Source Symbol Outline & Repo-Map Extractor) | Created: 2026-11-22
// Deep Production Verification Suite for lib/text/code_outline.cp
// Covers:
//   1. Multi-language (CP/Rust/TS) symbol extraction (import/const/enum/trait/class/struct/fn/test) & exact line ranges
//   2. Immunity to fake keywords and unbalanced braces inside // comments, /* */ comments, strings & Rust lifetimes
//   3. Nested method brace_depth tracking and local const filtering inside function bodies
//   4. findSymbolByName lookup & formatRepoMapLine formatting + buffer-too-small protection
//   5. Empty source, comment-only source, zero-length buffer & small-buffer truncation safety

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SymbolKind,
    CodeSymbolSpan,
    extractCodeOutline,
    findSymbolByName,
    formatRepoMapLine,
} from "../../lib/text/index.cp";

fn testMultiLanguageSymbolsAndExactLineRanges()! {
    const src =
        "import { foo } from \"cap:mcp\";\n" ++     // L1
        "pub const MAX_ITEMS: u32 = 64;\n" ++      // L2
        "pub enum Status {\n" ++                   // L3
        "    Ok,\n" ++                             // L4
        "    Err,\n" ++                            // L5
        "}\n" ++                                   // L6
        "pub trait Runner {\n" ++                  // L7
        "    fn step(): bool;\n" ++                // L8
        "}\n" ++                                   // L9
        "export class Worker {\n" ++               // L10
        "    async function run() {\n" ++          // L11
        "        return 1;\n" ++                   // L12
        "    }\n" ++                               // L13
        "}\n" ++                                   // L14
        "pub async fn serve(port: u16) {\n" ++     // L15
        "    const local_tmp = port;\n" ++         // L16 (local const ignored)
        "    * = local_tmp;\n" ++                  // L17
        "}\n" ++                                   // L18
        "test(\"smoke case\", serve);\n";          // L19

    let syms_buf: [16]CodeSymbolSpan = [CodeSymbolSpan{}; 16];
    let syms: []CodeSymbolSpan = syms_buf[0..];
    const total = extractCodeOutline(src, syms);

    // Expected 9 symbols: import, MAX_ITEMS, Status, Runner, step, Worker, run, serve, smoke case
    expect_eq(total, 9);

    expect_eq(syms[0].kind, SymbolKind.Import);
    expect_eq(std.mem.eql(u8, syms[0].nameSlice(src), "cap:mcp"), true);
    expect_eq(syms[0].start_line, 1);

    expect_eq(syms[1].kind, SymbolKind.Const);
    expect_eq(std.mem.eql(u8, syms[1].nameSlice(src), "MAX_ITEMS"), true);
    expect_eq(syms[1].is_public, true);
    expect_eq(syms[1].start_line, 2);

    expect_eq(syms[2].kind, SymbolKind.Enum);
    expect_eq(std.mem.eql(u8, syms[2].nameSlice(src), "Status"), true);
    expect_eq(syms[2].start_line, 3);
    expect_eq(syms[2].end_line, 6);

    expect_eq(syms[3].kind, SymbolKind.Trait);
    expect_eq(std.mem.eql(u8, syms[3].nameSlice(src), "Runner"), true);
    expect_eq(syms[3].start_line, 7);
    expect_eq(syms[3].end_line, 9);

    expect_eq(syms[4].kind, SymbolKind.Function);
    expect_eq(std.mem.eql(u8, syms[4].nameSlice(src), "step"), true);
    expect_eq(syms[4].brace_depth, 1);
    expect_eq(syms[4].start_line, 8);
    expect_eq(syms[4].end_line, 8);

    expect_eq(syms[5].kind, SymbolKind.Class);
    expect_eq(std.mem.eql(u8, syms[5].nameSlice(src), "Worker"), true);
    expect_eq(syms[5].is_public, true);
    expect_eq(syms[5].start_line, 10);
    expect_eq(syms[5].end_line, 14);

    expect_eq(syms[6].kind, SymbolKind.Function);
    expect_eq(std.mem.eql(u8, syms[6].nameSlice(src), "run"), true);
    expect_eq(syms[6].brace_depth, 1);
    expect_eq(syms[6].start_line, 11);
    expect_eq(syms[6].end_line, 13);

    expect_eq(syms[7].kind, SymbolKind.Function);
    expect_eq(std.mem.eql(u8, syms[7].nameSlice(src), "serve"), true);
    expect_eq(syms[7].is_public, true);
    expect_eq(syms[7].brace_depth, 0);
    expect_eq(syms[7].start_line, 15);
    expect_eq(syms[7].end_line, 18);

    expect_eq(syms[8].kind, SymbolKind.Test);
    expect_eq(std.mem.eql(u8, syms[8].nameSlice(src), "smoke case"), true);
    expect_eq(syms[8].start_line, 19);
}

fn testCommentStringAndLifetimeImmunity()! {
    const src =
        "// pub fn fakeLineComment() {\n" ++       // L1 (unclosed brace in comment)
        "/*\n" ++                                  // L2
        "pub struct FakeBlock {\n" ++              // L3
        "    fn fakeInner() {}\n" ++               // L4
        "}\n" ++                                   // L5
        "*/\n" ++                                  // L6
        "pub fn realBorrow<'a>(s: &'a str) {\n" ++ // L7 (Rust lifetime 'a)
        "    const a = \"fn fakeStr() { }\";\n" ++ // L8
        "    const b = 'class FakeSingle {';\n" ++ // L9
        "    const c = `enum FakeTpl {\n" ++       // L10
        "        trait FakeMulti {}\n" ++          // L11
        "    }`;\n" ++                             // L12
        "}\n" ++                                   // L13
        "struct AfterAll {\n" ++                   // L14
        "    x: u32,\n" ++                         // L15
        "}\n";                                     // L16

    let syms_buf: [8]CodeSymbolSpan = [CodeSymbolSpan{}; 8];
    let syms: []CodeSymbolSpan = syms_buf[0..];
    const total = extractCodeOutline(src, syms);

    // Must extract ONLY realBorrow (L7-L13) and AfterAll (L14-L16)
    expect_eq(total, 2);

    expect_eq(syms[0].kind, SymbolKind.Function);
    expect_eq(std.mem.eql(u8, syms[0].nameSlice(src), "realBorrow"), true);
    expect_eq(syms[0].is_public, true);
    expect_eq(syms[0].start_line, 7);
    expect_eq(syms[0].end_line, 13);

    expect_eq(syms[1].kind, SymbolKind.Struct);
    expect_eq(std.mem.eql(u8, syms[1].nameSlice(src), "AfterAll"), true);
    expect_eq(syms[1].is_public, false);
    expect_eq(syms[1].start_line, 14);
    expect_eq(syms[1].end_line, 16);
}

fn testNestedStructMethodsAndBraceDepth()! {
    const src =
        "pub struct Engine {\n" ++                  // L1 (depth 0)
        "    pub static const VERSION: u32 = 2;\n" ++ // L2 (depth 1, modified const)
        "    static fn init() {\n" ++               // L3 (depth 1)
        "        const inner_var = 1;\n" ++         // L4 (local const ignored)
        "        if (true) {\n" ++                  // L5 (depth 2)
        "            * = inner_var;\n" ++           // L6
        "        }\n" ++                            // L7
        "    }\n" ++                                // L8
        "    fn execute() {\n" ++                   // L9 (depth 1)
        "        return;\n" ++                      // L10
        "    }\n" ++                                // L11
        "}\n";                                      // L12

    let syms_buf: [8]CodeSymbolSpan = [CodeSymbolSpan{}; 8];
    let syms: []CodeSymbolSpan = syms_buf[0..];
    const total = extractCodeOutline(src, syms);

    expect_eq(total, 4);

    expect_eq(std.mem.eql(u8, syms[0].nameSlice(src), "Engine"), true);
    expect_eq(syms[0].brace_depth, 0);
    expect_eq(syms[0].start_line, 1);
    expect_eq(syms[0].end_line, 12);

    expect_eq(std.mem.eql(u8, syms[1].nameSlice(src), "VERSION"), true);
    expect_eq(syms[1].kind, SymbolKind.Const);
    expect_eq(syms[1].brace_depth, 1);
    expect_eq(syms[1].is_public, true);

    expect_eq(std.mem.eql(u8, syms[2].nameSlice(src), "init"), true);
    expect_eq(syms[2].brace_depth, 1);
    expect_eq(syms[2].start_line, 3);
    expect_eq(syms[2].end_line, 8);

    expect_eq(std.mem.eql(u8, syms[3].nameSlice(src), "execute"), true);
    expect_eq(syms[3].brace_depth, 1);
    expect_eq(syms[3].start_line, 9);
    expect_eq(syms[3].end_line, 11);
}

fn testFindSymbolByNameAndFormatRepoMapLine()! {
    const src =
        "struct Helper {\n" ++                      // L1
        "    v: u32,\n" ++                          // L2
        "}\n" ++                                    // L3
        "pub fn applyUnifiedPatch() {\n" ++         // L4
        "    return;\n" ++                          // L5
        "}\n";                                      // L6

    let syms_buf: [4]CodeSymbolSpan = [CodeSymbolSpan{}; 4];
    let syms: []CodeSymbolSpan = syms_buf[0..];
    const total = extractCodeOutline(src, syms);
    expect_eq(total, 2);

    // Lookup existing symbols
    const s1 = findSymbolByName(src, syms[0..total], "applyUnifiedPatch") ?? CodeSymbolSpan{};
    expect_eq(s1.start_line, 4);
    expect_eq(s1.end_line, 6);
    expect_eq(s1.is_public, true);

    const s2 = findSymbolByName(src, syms[0..total], "Helper") ?? CodeSymbolSpan{};
    expect_eq(s2.start_line, 1);
    expect_eq(s2.end_line, 3);
    expect_eq(s2.is_public, false);

    // Lookup absent or empty target returns null
    expect_eq(findSymbolByName(src, syms[0..total], "doesNotExist") == null, true);
    expect_eq(findSymbolByName(src, syms[0..total], "") == null, true);

    // Format Repo-Map lines
    let line_buf: [64]u8 = [0; 64];
    let line: []u8 = line_buf[0..];
    const n1 = formatRepoMapLine(src, s1, line) ?? 0;
    expect_eq(std.mem.eql(u8, line[0..n1], "pub fn applyUnifiedPatch [L4-L6]"), true);

    const n2 = formatRepoMapLine(src, s2, line) ?? 0;
    expect_eq(std.mem.eql(u8, line[0..n2], "struct Helper [L1-L3]"), true);

    // Undersized format buffer returns null cleanly
    let tiny_buf: [8]u8 = [0; 8];
    let tiny: []u8 = tiny_buf[0..];
    expect_eq(formatRepoMapLine(src, s1, tiny) == null, true);
}

fn testEmptySourceAndTruncationSafety()! {
    let syms_buf: [2]CodeSymbolSpan = [CodeSymbolSpan{}; 2];
    let syms: []CodeSymbolSpan = syms_buf[0..];

    // 1. Empty source -> 0
    expect_eq(extractCodeOutline("", syms), 0);

    // 2. Comment-only source -> 0
    expect_eq(extractCodeOutline("// nothing here\n/* still nothing */\n", syms), 0);

    const four_fns =
        "fn f1() {\n}\n" ++
        "fn f2() {\n}\n" ++
        "fn f3() {\n}\n" ++
        "fn f4() {\n}\n";

    // 3. Zero-length output slice safely counts all 4 symbols without writing
    let zero_buf: [0]CodeSymbolSpan = [];
    let zero_slice: []CodeSymbolSpan = zero_buf[0..];
    expect_eq(extractCodeOutline(four_fns, zero_slice), 4);

    // 4. Capacity-2 buffer writes first 2 symbols with accurate end_line and returns total=4
    const total = extractCodeOutline(four_fns, syms);
    expect_eq(total, 4);
    expect_eq(std.mem.eql(u8, syms[0].nameSlice(four_fns), "f1"), true);
    expect_eq(syms[0].start_line, 1);
    expect_eq(syms[0].end_line, 2);
    expect_eq(std.mem.eql(u8, syms[1].nameSlice(four_fns), "f2"), true);
    expect_eq(syms[1].start_line, 3);
    expect_eq(syms[1].end_line, 4);
}

test("text code_outline: multi-language symbol extraction and exact line ranges", testMultiLanguageSymbolsAndExactLineRanges);
test("text code_outline: comment, string literal and Rust lifetime immunity", testCommentStringAndLifetimeImmunity);
test("text code_outline: nested struct methods brace_depth and local const filtering", testNestedStructMethodsAndBraceDepth);
test("text code_outline: findSymbolByName and formatRepoMapLine formatting", testFindSymbolByNameAndFormatRepoMapLine);
test("text code_outline: empty source, zero-length buffer and truncation safety", testEmptySourceAndTruncationSafety);
