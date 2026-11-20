// Source: cp/cap/test/test_text_production_deep.cp
// Deep Production Verification Suite for cap:text & cap.text
// Covers:
//   1. TextChunker.splitRecursive paragraph/sentence/space splitting, overlap window & UTF-8 zero-split safety
//   2. TextChunker.splitMarkdown heading preservation & fenced-code-block safety
//   3. levenshtein, damerauLevenshtein, jaroWinkler, fuzzyScore & fuzzyTopK ranking
//   4. TokenEstimator.analyze, estimateTokens & UTF-8 safe truncateToTokenBudget
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.text`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TextErr,
    TextChunk,
    ChunkOptions,
    TextChunker,
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaroWinkler,
    fuzzyScore,
    fuzzyTopK,
    TextStats,
    TokenEstimator,
} from "../../lib/text/index.cp";

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn countUtf8Codepoints(s: string): usize {
    let i: usize = 0;
    let count: usize = 0;
    while (i < s.len) {
        const step = std.unicode.utf8ByteSequenceLength(s[i]) catch 1;
        i += step;
        count += 1;
    }
    return count;
}

fn testRecursiveChunkerAndUtf8Safety()! {
    const alloc = std.heap.page_allocator;

    // 1. Invalid max_chars == 0 returns TextErr.InvalidParameter
    let got_invalid = false;
    * = TextChunker.splitRecursive(alloc, "hello", ChunkOptions{ max_chars: 0, overlap_chars: 0 }) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidParameter")) got_invalid = true;
    };
    expect_eq(got_invalid, true);

    // 2. Empty input returns 0 chunks cleanly
    const empty_chunks = TextChunker.splitRecursive(alloc, "", ChunkOptions{ max_chars: 16, overlap_chars: 0 })?;
    defer alloc.free(empty_chunks);
    expect_eq(empty_chunks.len, 0);

    // 3. Paragraph (\n\n) -> Sentence (. ) -> Space ( ) priority splitting
    const doc = "First paragraph here.\n\nSecond paragraph. Third sentence follows.\n\nEnd.";
    const para_chunks = TextChunker.splitRecursive(alloc, doc, ChunkOptions{ max_chars: 25, overlap_chars: 0 })?;
    defer alloc.free(para_chunks);

    expect_eq(para_chunks.len >= 3, true);
    expect_eq(std.mem.eql(u8, para_chunks[0].text, "First paragraph here."), true);
    expect_eq(para_chunks[0].chunk_index, 0);
    expect_eq(std.mem.eql(u8, doc[para_chunks[0].start_byte..para_chunks[0].end_byte], para_chunks[0].text), true);

    for (c of para_chunks) {
        expect_eq(std.unicode.utf8ValidateSlice(c.text), true);
        expect_eq(countUtf8Codepoints(c.text) <= 25, true);
        expect_eq(std.mem.eql(u8, doc[c.start_byte..c.end_byte], c.text), true);
    }

    // 4. Multi-byte Chinese + Emoji continuous string with overlap window (no spaces)
    const utf8_text = "你好世界人工智能大模型上下文分块测试🚀✨";
    const utf_chunks = TextChunker.splitRecursive(alloc, utf8_text, ChunkOptions{ max_chars: 6, overlap_chars: 2 })?;
    defer alloc.free(utf_chunks);

    expect_eq(utf_chunks.len >= 4, true);
    // Chunk 0: "你好世界人工" (6 codepoints, 18 bytes)
    expect_eq(std.mem.eql(u8, utf_chunks[0].text, "你好世界人工"), true);
    expect_eq(utf_chunks[0].start_byte, 0);
    expect_eq(utf_chunks[0].end_byte, 18);

    // Chunk 1 overlaps last 2 codepoints ("人工"): starts with "人工"
    expect_eq(std.mem.startsWith(u8, utf_chunks[1].text, "人工"), true);

    let idx: usize = 0;
    while (idx < utf_chunks.len) : (idx += 1) {
        const ch = utf_chunks[idx];
        expect_eq(ch.chunk_index, idx);
        expect_eq(std.unicode.utf8ValidateSlice(ch.text), true);
        expect_eq(countUtf8Codepoints(ch.text) <= 6, true);
        expect_eq(std.mem.eql(u8, utf8_text[ch.start_byte..ch.end_byte], ch.text), true);
    }
}

fn testMarkdownChunkerHeadingPreservation()! {
    const alloc = std.heap.page_allocator;

    const md = "Preamble text.\n\n# Introduction\nWelcome to CP RAG.\n\n## Code Example\n```python\n# not a markdown heading\nx = 1\n```\n\n### Deep Section\nAlpha beta gamma delta epsilon zeta eta theta iota kappa.";
    const chunks = TextChunker.splitMarkdown(alloc, md, 48)?;
    defer alloc.free(chunks);

    expect_eq(chunks.len >= 5, true);
    expect_eq(std.mem.eql(u8, chunks[0].heading, ""), true);
    expect_eq(std.mem.eql(u8, chunks[0].text, "Preamble text."), true);

    expect_eq(std.mem.eql(u8, chunks[1].heading, "Introduction"), true);
    expect_eq(std.mem.indexOf(u8, chunks[1].text, "Welcome to CP RAG.") != null, true);

    // Fenced `# not a markdown heading` must stay inside "Code Example"
    let saw_code_example = false;
    let saw_bogus_heading = false;
    let deep_count: usize = 0;
    let idx: usize = 0;
    while (idx < chunks.len) : (idx += 1) {
        const c = chunks[idx];
        expect_eq(c.chunk_index, idx);
        expect_eq(std.unicode.utf8ValidateSlice(c.text), true);
        expect_eq(countUtf8Codepoints(c.text) <= 48, true);
        expect_eq(std.mem.eql(u8, md[c.start_byte..c.end_byte], c.text), true);
        if (std.mem.eql(u8, c.heading, "Code Example")) saw_code_example = true;
        if (std.mem.eql(u8, c.heading, "not a markdown heading")) saw_bogus_heading = true;
        if (std.mem.eql(u8, c.heading, "Deep Section")) deep_count += 1;
    }
    expect_eq(saw_code_example, true);
    expect_eq(saw_bogus_heading, false);
    expect_eq(deep_count >= 2, true);
}

fn testFuzzyDistancesAndTopK()! {
    const alloc = std.heap.page_allocator;

    // 1. Levenshtein (ASCII + UTF-8 codepoints)
    expect_eq(levenshtein(alloc, "kitten", "sitting")?, 3);
    expect_eq(levenshtein(alloc, "café", "cafe")?, 1);
    expect_eq(levenshtein(alloc, "你好世界", "你我世界")?, 1);
    expect_eq(levenshtein(alloc, "", "abc")?, 3);

    // 2. True Damerau-Levenshtein (transpositions + Lowrance-Wagner "ca" <-> "abc" == 2)
    expect_eq(damerauLevenshtein(alloc, "ab", "ba")?, 1);
    expect_eq(damerauLevenshtein(alloc, "ca", "abc")?, 2);
    expect_eq(damerauLevenshtein(alloc, "algoritmh", "algorithm")?, 1);

    // 3. Zero-alloc Jaro-Winkler
    expect_eq(approxF64(jaroWinkler("", ""), 1.0, 0.0001), true);
    expect_eq(approxF64(jaroWinkler("abc", ""), 0.0, 0.0001), true);
    expect_eq(approxF64(jaroWinkler("MARTHA", "MARHTA"), 0.961111, 0.0005), true);
    expect_eq(approxF64(jaroWinkler("DIXON", "DICKSONX"), 0.813333, 0.0005), true);

    // 4. fuzzyScore bonuses & non-match
    expect_eq(fuzzyScore("xyz", "searchFiles") == null, true);
    const s_camel = fuzzyScore("sf", "searchFiles").?;
    const s_snake = fuzzyScore("sf", "save_file").?;
    const s_plain = fuzzyScore("sf", "satisfy").?;
    expect_eq(s_camel > s_plain, true);
    expect_eq(s_snake > s_plain, true);

    const s_exact = fuzzyScore("foo", "foo").?;
    const s_prefix = fuzzyScore("foo", "foobar").?;
    expect_eq(s_exact > s_prefix, true);

    // 5. fuzzyTopK ranking & truncation
    const candidates: [6]string = [
        "satisfy",
        "closeWindow",
        "searchFiles",
        "save_file",
        "sf",
        "openFolder",
    ];
    const top3 = fuzzyTopK(alloc, "sf", &candidates, 3)?;
    defer alloc.free(top3);

    expect_eq(top3.len, 3);
    expect_eq(std.mem.eql(u8, top3[0].text, "sf"), true);
    expect_eq(top3[0].index, 4);
    expect_eq(top3[0].score >= top3[1].score, true);
    expect_eq(top3[1].score >= top3[2].score, true);

    const top0 = fuzzyTopK(alloc, "sf", &candidates, 0)?;
    defer alloc.free(top0);
    expect_eq(top0.len, 0);
}

fn testTokenEstimatorAndUtf8BudgetTruncation()! {
    const empty_stats = TokenEstimator.analyze("");
    expect_eq(empty_stats.bytes, 0);
    expect_eq(empty_stats.codepoints, 0);
    expect_eq(empty_stats.words, 0);
    expect_eq(empty_stats.lines, 0);
    expect_eq(empty_stats.cjk_chars, 0);
    expect_eq(empty_stats.estimated_tokens, 0);

    const sample = "Agent v2.0: 你好世界!\nLine two 123456.";
    const st = TokenEstimator.analyze(sample);
    expect_eq(st.bytes, sample.len);
    expect_eq(st.lines, 2);
    expect_eq(st.cjk_chars, 4);
    expect_eq(st.words, 6);
    expect_eq(st.estimated_tokens, TokenEstimator.estimateTokens(sample));
    expect_eq(st.estimated_tokens >= 10, true);

    // Zero budget & full budget
    expect_eq(TokenEstimator.truncateToTokenBudget(sample, 0).len, 0);
    expect_eq(std.mem.eql(u8, TokenEstimator.truncateToTokenBudget(sample, 100), sample), true);

    // Sweeping budgets over mixed ASCII + CJK + Emoji: never split UTF-8 and always <= budget
    const mixed = "Prompt: 计算智能体 Token 预算 🚀 123456789";
    const budgets: [6]usize = [1, 2, 4, 7, 10, 14];
    for (b of budgets) {
        const cut = TokenEstimator.truncateToTokenBudget(mixed, b);
        expect_eq(std.unicode.utf8ValidateSlice(cut), true);
        expect_eq(std.mem.startsWith(u8, mixed, cut), true);
        expect_eq(TokenEstimator.estimateTokens(cut) <= b, true);
    }
}

fn testCapTextDynamicMount()! {
    const alloc = std.heap.page_allocator;

    const rag_doc = "# Memory Core\nRAG pipeline splits long documents cleanly.\n\n## Search\nFuzzy search finds commands fast.";
    const chunks = cap.text.TextChunker.splitMarkdown(alloc, rag_doc, 60)?;
    defer alloc.free(chunks);
    expect_eq(chunks.len >= 2, true);
    expect_eq(std.mem.eql(u8, chunks[0].heading, "Memory Core"), true);

    const budget_text = cap.text.TokenEstimator.truncateToTokenBudget(chunks[0].text, 6);
    expect_eq(std.unicode.utf8ValidateSlice(budget_text), true);
    expect_eq(cap.text.TokenEstimator.estimateTokens(budget_text) <= 6, true);

    const cmds: [3]string = ["memoryCore", "memoryClear", "openTab"];
    const matches = cap.text.fuzzyTopK(alloc, "mc", &cmds, 2)?;
    defer alloc.free(matches);
    expect_eq(matches.len, 2);
    expect_eq(cap.text.levenshtein(alloc, "rag", "rag")?, 0);
    expect_eq(cap.text.jaroWinkler("memory", "memory") > 0.99, true);
}

fn main() {
    test("1. TextChunker.splitRecursive Paragraphs, Overlap Window & Multi-Byte UTF-8 Safety", () => {
        testRecursiveChunkerAndUtf8Safety()?;
    });
    test("2. TextChunker.splitMarkdown Heading Preservation & Fenced Code Safety", () => {
        testMarkdownChunkerHeadingPreservation()?;
    });
    test("3. Levenshtein, Damerau-Levenshtein, Jaro-Winkler, fuzzyScore & fuzzyTopK", () => {
        testFuzzyDistancesAndTopK()?;
    });
    test("4. TokenEstimator.analyze, estimateTokens & UTF-8 Safe truncateToTokenBudget", () => {
        testTokenEstimatorAndUtf8BudgetTruncation()?;
    });
    test("5. import cap from \"cap\"; cap.text Automatic Dynamic Lazy Mounting", () => {
        testCapTextDynamicMount()?;
    });
}
