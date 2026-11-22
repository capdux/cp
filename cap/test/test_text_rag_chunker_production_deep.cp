// Origin: Pure-CP Stdlib Test Suite (lib/text/rag_chunker.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation RAG recursive Markdown-aware document chunker,
// code-fence integrity, sliding overlap windows, UTF-8 codepoint boundary safety, CJK token density,
// 1-based line citation spans, and defensive parameter guards.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RagChunkConfig,
    RagChunkSpan,
    chunkDocumentRag,
    estimateRagTokens,
    locateLineRange,
} from "cap:text";

fn isValidUtf8Boundary(s: string, pos: usize): bool {
    if (pos == 0 || pos == s.len) {
        return true;
    }
    if (pos > s.len) {
        return false;
    }
    return (s[pos] & 0xC0) != 0x80;
}

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit: usize = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn testMarkdownHeadingsAndParagraphSplitting()! {
    const doc = "# Intro\nFirst paragraph line.\n\nSecond paragraph details.\n\n## Architecture\nCore engine overview here.\n\n## Usage\nCall chunkDocumentRag directly.\n";
    let spans_arr: [16]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_arr[0..];

    const cfg = RagChunkConfig{
        max_bytes: 70,
        overlap_bytes: 0,
        respect_code_fences: true,
        split_on_headings: true,
    };

    if (n := chunkDocumentRag(doc, cfg, spans)) {
        expect_eq(n >= 2, true);
        // First chunk ends cleanly before "## Architecture"
        expect_eq(spans[0].index, 0);
        expect_eq(spans[0].line_start, 1);
        expect_eq(containsSubstr(spans[0].slice(doc), "# Intro"), true);
        // Second chunk starts right at "## Architecture"
        expect_eq(spans[1].slice(doc)[0..2], "##");
        // Final chunk reaches exact end of document
        expect_eq(spans[n - 1].end, doc.len);
        // All spans have valid sequential indices & non-zero est_tokens
        let i: usize = 0;
        while (i < n) : (i += 1) {
            expect_eq(spans[i].index, i as u32);
            expect_eq(spans[i].est_tokens > 0, true);
            expect_eq(spans[i].line_start <= spans[i].line_end, true);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testCodeFenceIntegrityProtection()! {
    const doc = "Brief explanation before code.\n\n```cp\nfn add(a: i32, b: i32): i32 {\n    return a + b;\n}\n```\n\nSummary after code block.";
    let spans_arr: [8]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_arr[0..];

    // max_bytes=75 would normally bisect the code fence right in the middle!
    // With respect_code_fences=true, chunk 0 splits right before ```cp so chunk 1 keeps the entire fence intact.
    const cfg = RagChunkConfig{
        max_bytes: 75,
        overlap_bytes: 0,
        respect_code_fences: true,
        split_on_headings: true,
    };

    if (n := chunkDocumentRag(doc, cfg, spans)) {
        expect_eq(n >= 2, true);
        const c0 = spans[0].slice(doc);
        const c1 = spans[1].slice(doc);
        expect_eq(containsSubstr(c0, "```"), false);
        expect_eq(containsSubstr(c1, "```cp\nfn add"), true);
        expect_eq(containsSubstr(c1, "return a + b;\n}\n```"), true);
    } else {
        expect_eq(true, false);
    }
}

fn testSlidingOverlapWindowContext()! {
    const doc = "Alpha one two three four. Beta five six seven eight. Gamma nine ten eleven twelve. Delta thirteen fourteen fifteen.";
    let spans_arr: [8]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_arr[0..];

    const cfg = RagChunkConfig{
        max_bytes: 48,
        overlap_bytes: 16,
        respect_code_fences: true,
        split_on_headings: false,
    };

    if (n := chunkDocumentRag(doc, cfg, spans)) {
        expect_eq(n >= 3, true);
        let i: usize = 1;
        while (i < n) : (i += 1) {
            // Next chunk must start strictly after previous start and strictly before previous end (overlap!)
            expect_eq(spans[i].start > spans[i - 1].start, true);
            expect_eq(spans[i].start < spans[i - 1].end, true);
            // Overlap region bytes must match exactly
            const overlap_len: usize = spans[i - 1].end - spans[i].start;
            expect_eq(overlap_len > 0, true);
            expect_eq(overlap_len <= cfg.overlap_bytes, true);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testUtf8MultibyteCjkAndEmojiSafety()! {
    // Mixed Chinese (3-byte UTF-8) and Emoji (4-byte UTF-8) with no ASCII spaces
    const doc = "人工智能检索增强生成系统🚀✨支持零堆分配分块。每个分块必须严格对齐合法编码边界🔥绝不截断半个字符。";
    let spans_arr: [16]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_arr[0..];

    // Deliberately choose odd max_bytes & overlap_bytes that land inside 3-byte/4-byte sequences
    const cfg = RagChunkConfig{
        max_bytes: 29,
        overlap_bytes: 7,
        respect_code_fences: true,
        split_on_headings: true,
    };

    if (n := chunkDocumentRag(doc, cfg, spans)) {
        expect_eq(n >= 4, true);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            expect_eq(isValidUtf8Boundary(doc, spans[i].start), true);
            expect_eq(isValidUtf8Boundary(doc, spans[i].end), true);
            expect_eq(spans[i].start < spans[i].end, true);
        }
        expect_eq(spans[0].start, 0);
        expect_eq(spans[n - 1].end, doc.len);
    } else {
        expect_eq(true, false);
    }
}

fn testEstimateRagTokensAndLocateLineRange()! {
    // 1. Token density: CJK vs ASCII of identical byte length (30 bytes each)
    const zh_30b = "人工智能检索增强生成"; // 10 CJK chars = 30 bytes -> ~10 tokens
    const en_30b = "artificial intelligence rag ok"; // 4 ASCII words = 30 bytes -> ~7 tokens
    const t_zh = estimateRagTokens(zh_30b);
    const t_en = estimateRagTokens(en_30b);
    expect_eq(t_zh, 10);
    expect_eq(t_zh > t_en, true);
    expect_eq(estimateRagTokens(""), 0);
    expect_eq(estimateRagTokens("   \n\t  "), 0);

    // 2. 1-based line range locator
    const lines_doc = "line1\nline2\nline3\nline4";
    const r1 = locateLineRange(lines_doc, 0, 5);   // "line1"
    expect_eq(r1[0], 1);
    expect_eq(r1[1], 1);

    const r2 = locateLineRange(lines_doc, 0, 6);   // "line1\n" (trailing newline stays on line 1)
    expect_eq(r2[0], 1);
    expect_eq(r2[1], 1);

    const r3 = locateLineRange(lines_doc, 6, 17);  // "line2\nline3"
    expect_eq(r3[0], 2);
    expect_eq(r3[1], 3);

    const r4 = locateLineRange(lines_doc, 18, 23); // "line4"
    expect_eq(r4[0], 4);
    expect_eq(r4[1], 4);
}

fn testDefensiveGuardsAndCapNamespace()! {
    let spans_arr: [2]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_arr[0..];
    const def_cfg = cap.text.RagChunkConfig.default();

    // 1. max_bytes == 0 -> null
    const bad_zero = RagChunkConfig{ max_bytes: 0, overlap_bytes: 0, respect_code_fences: true, split_on_headings: true };
    expect_eq(chunkDocumentRag("hello", bad_zero, spans) == null, true);

    // 2. overlap_bytes >= max_bytes -> null
    const bad_ov1 = RagChunkConfig{ max_bytes: 32, overlap_bytes: 32, respect_code_fences: true, split_on_headings: true };
    const bad_ov2 = RagChunkConfig{ max_bytes: 32, overlap_bytes: 40, respect_code_fences: true, split_on_headings: true };
    expect_eq(chunkDocumentRag("hello world", bad_ov1, spans) == null, true);
    expect_eq(chunkDocumentRag("hello world", bad_ov2, spans) == null, true);

    // 3. Empty document returns 0 cleanly (even with empty output buffer)
    let zero_slice: []RagChunkSpan = spans_arr[0..0];
    if (n := cap.text.chunkDocumentRag("", def_cfg, zero_slice)) {
        expect_eq(n, 0);
    } else {
        expect_eq(true, false);
    }

    // 4. Insufficient output buffer returns null cleanly
    const long_doc = "One two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen.";
    const tiny_cfg = RagChunkConfig{ max_bytes: 20, overlap_bytes: 0, respect_code_fences: false, split_on_headings: false };
    expect_eq(chunkDocumentRag(long_doc, tiny_cfg, spans) == null, true);

    // 5. Dynamic cap.text.* namespace verification
    const lr = cap.text.locateLineRange("a\nb\nc", 2, 5);
    expect_eq(lr[0], 2);
    expect_eq(lr[1], 3);
    expect_eq(cap.text.estimateRagTokens("hello world"), 2);
}

fn main() {
    test("1. text/rag_chunker: Markdown headings & paragraph recursive splitting", () => {
        testMarkdownHeadingsAndParagraphSplitting()?;
    });

    test("2. text/rag_chunker: fenced code block integrity protection", () => {
        testCodeFenceIntegrityProtection()?;
    });

    test("3. text/rag_chunker: sliding overlap window preserves tail context", () => {
        testSlidingOverlapWindowContext()?;
    });

    test("4. text/rag_chunker: UTF-8 multibyte CJK & Emoji codepoint boundary safety", () => {
        testUtf8MultibyteCjkAndEmojiSafety()?;
    });

    test("5. text/rag_chunker: CJK-aware estimateRagTokens & 1-based locateLineRange", () => {
        testEstimateRagTokensAndLocateLineRange()?;
    });

    test("6. text/rag_chunker: defensive guards (max=0, overlap>=max, small buf) & cap.text", () => {
        testDefensiveGuardsAndCapNamespace()?;
    });
}
