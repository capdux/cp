// Origin: Pure-CP Stdlib Test Suite (AI RAG Structure-Aware Markdown Chunker & Heading Breadcrumb Indexer) | Created: 2026-11-22
// Deep Production Verification Suite for lib/text/markdown_chunk.cp
// Covers:
//   1. 6-level ATX heading breadcrumb tracking, level reset & formatBreadcrumb buffer guard
//   2. Fenced code block (``` / ~~~) integrity preservation & '#' comment non-heading immunity
//   3. GFM table integrity preservation, list detection & configurable table splitting
//   4. Oversized prose splitting (UTF-8 safe), short prose merging & exact 1-based line numbers
//   5. Empty/whitespace/heading-only docs, zero max_chars & small output slice truncation safety

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MdSectionKind,
    MdChunkConfig,
    MdHeadingTrail,
    MdChunkSpan,
    chunkMarkdownStructured,
} from "../../lib/text/index.cp";

fn testHeadingBreadcrumbHierarchy()! {
    const doc = "# Intro\nOverview paragraph.\n\n## Setup\n### Install ###\nRun cap install.\n\n## Usage\nInvoke runtime.\n\n#### Deep Note\nDetailed hint.";
    const cfg = MdChunkConfig{
        max_chars: 256,
        min_chars: 16,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };

    let chunks_buf: [8]MdChunkSpan = [MdChunkSpan{}; 8];
    let chunks: []MdChunkSpan = chunks_buf[0..];
    const n = chunkMarkdownStructured(doc, cfg, chunks);
    expect_eq(n, 4);

    let crumb_buf: [128]u8 = [0; 128];
    let crumb: []u8 = crumb_buf[0..];

    // Chunk 0: Intro
    expect_eq(chunks[0].heading_level, 1);
    expect_eq(chunks[0].trail.depth(), 1);
    const len0 = chunks[0].trail.formatBreadcrumb(doc, crumb) ?? 0;
    expect_eq(std.mem.eql(u8, crumb[0..len0], "Intro"), true);
    expect_eq(std.mem.eql(u8, chunks[0].slice(doc), "Overview paragraph."), true);

    // Chunk 1: Intro > Setup > Install
    expect_eq(chunks[1].heading_level, 3);
    expect_eq(chunks[1].trail.depth(), 3);
    const len1 = chunks[1].trail.formatBreadcrumb(doc, crumb) ?? 0;
    expect_eq(std.mem.eql(u8, crumb[0..len1], "Intro > Setup > Install"), true);
    expect_eq(std.mem.eql(u8, chunks[1].trail.headingText(doc, 3) ?? "", "Install"), true);

    // Undersized breadcrumb buffer returns null
    let tiny_buf: [8]u8 = [0; 8];
    let tiny: []u8 = tiny_buf[0..];
    expect_eq(chunks[1].trail.formatBreadcrumb(doc, tiny) == null, true);

    // Chunk 2: Intro > Usage (H3 cleared when H2 updates)
    expect_eq(chunks[2].heading_level, 2);
    expect_eq(chunks[2].trail.depth(), 2);
    expect_eq(chunks[2].trail.headingText(doc, 3) == null, true);
    const len2 = chunks[2].trail.formatBreadcrumb(doc, crumb) ?? 0;
    expect_eq(std.mem.eql(u8, crumb[0..len2], "Intro > Usage"), true);

    // Chunk 3: Intro > Usage > Deep Note (skips absent H3)
    expect_eq(chunks[3].heading_level, 4);
    expect_eq(chunks[3].trail.depth(), 3);
    const len3 = chunks[3].trail.formatBreadcrumb(doc, crumb) ?? 0;
    expect_eq(std.mem.eql(u8, crumb[0..len3], "Intro > Usage > Deep Note"), true);
}

fn testCodeBlockFenceAndHashImmunity()! {
    const doc = "# Scripts\n## Build\n```python\n# not a heading\n## also a comment\ndef build():\n    return 42\n```\n\n~~~bash\n# shell comment\necho ok\n~~~\n\nAfter code paragraph.";
    const intact_cfg = MdChunkConfig{
        max_chars: 32,
        min_chars: 8,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };

    let chunks_buf: [8]MdChunkSpan = [MdChunkSpan{}; 8];
    let chunks: []MdChunkSpan = chunks_buf[0..];
    const n = chunkMarkdownStructured(doc, intact_cfg, chunks);
    expect_eq(n, 3);

    // First fenced block (```python) kept intact despite exceeding max_chars=32
    expect_eq(chunks[0].kind == MdSectionKind.CodeBlock, true);
    expect_eq(chunks[0].start_line, 3);
    expect_eq(chunks[0].end_line, 8);
    expect_eq(chunks[0].slice(doc).len > 32, true);

    // Second fenced block (~~~bash) kept intact
    expect_eq(chunks[1].kind == MdSectionKind.CodeBlock, true);
    expect_eq(chunks[1].start_line, 10);
    expect_eq(chunks[1].end_line, 13);

    // Trailing prose still has exact breadcrumb "Scripts > Build" (immune to '#' inside code)
    expect_eq(chunks[2].kind == MdSectionKind.Prose, true);
    expect_eq(chunks[2].heading_level, 2);
    let crumb_buf: [64]u8 = [0; 64];
    let crumb: []u8 = crumb_buf[0..];
    const clen = chunks[2].trail.formatBreadcrumb(doc, crumb) ?? 0;
    expect_eq(std.mem.eql(u8, crumb[0..clen], "Scripts > Build"), true);

    // When keep_code_blocks_intact == false, oversized code block is split
    const split_cfg = MdChunkConfig{
        max_chars: 32,
        min_chars: 8,
        keep_code_blocks_intact: false,
        keep_tables_intact: true,
    };
    const n_split = chunkMarkdownStructured(doc, split_cfg, chunks);
    expect_eq(n_split > 3, true);
    expect_eq(chunks[0].kind == MdSectionKind.CodeBlock, true);
    expect_eq(chunks[0].slice(doc).len <= 32, true);
}

fn testTableIntegrityAndListSection()! {
    const doc = "# Spec\n| Name | Value | Notes |\n| :--- | :---: | ---: |\n| alpha | 100 | first row |\n| beta | 200 | second row |\n\n- item one\n- item two\n  continued detail";
    const cfg = MdChunkConfig{
        max_chars: 40,
        min_chars: 8,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };

    let chunks_buf: [8]MdChunkSpan = [MdChunkSpan{}; 8];
    let chunks: []MdChunkSpan = chunks_buf[0..];
    const n = chunkMarkdownStructured(doc, cfg, chunks);
    expect_eq(n, 2);

    // Entire 4-line GFM table preserved as a single Table chunk
    expect_eq(chunks[0].kind == MdSectionKind.Table, true);
    expect_eq(chunks[0].start_line, 2);
    expect_eq(chunks[0].end_line, 5);
    expect_eq(chunks[0].slice(doc).len > 40, true);

    // List block recognized as MdSectionKind.List
    expect_eq(chunks[1].kind == MdSectionKind.List, true);
    expect_eq(chunks[1].start_line, 7);
    expect_eq(chunks[1].end_line, 9);

    // Allowing table splitting breaks oversized table into multiple Table chunks
    const split_tbl_cfg = MdChunkConfig{
        max_chars: 40,
        min_chars: 8,
        keep_code_blocks_intact: true,
        keep_tables_intact: false,
    };
    const n_split = chunkMarkdownStructured(doc, split_tbl_cfg, chunks);
    expect_eq(n_split > 2, true);
    expect_eq(chunks[0].kind == MdSectionKind.Table, true);
    expect_eq(chunks[0].slice(doc).len <= 40, true);
}

fn testLongProseSplitMergeAndLineNumbers()! {
    // 1. Oversized prose with UTF-8 CJK characters split safely within max_chars=40
    const long_doc = "# Guide\nFirst line with multiple words for splitting.\n第二行包含中文字符用于验证UTF8码点边界安全切分。\nThird line closes the paragraph.";
    const split_cfg = MdChunkConfig{
        max_chars: 40,
        min_chars: 8,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };
    let chunks_buf: [16]MdChunkSpan = [MdChunkSpan{}; 16];
    let chunks: []MdChunkSpan = chunks_buf[0..];
    const n = chunkMarkdownStructured(long_doc, split_cfg, chunks);
    expect_eq(n >= 4, true);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const sl = chunks[i].slice(long_doc);
        expect_eq(chunks[i].kind == MdSectionKind.Prose, true);
        expect_eq(sl.len > 0 && sl.len <= 40, true);
        // Must never start on a UTF-8 continuation byte (10xxxxxx)
        expect_eq((sl[0] & 0xC0) != 0x80, true);
        expect_eq(chunks[i].start_line <= chunks[i].end_line, true);
    }
    expect_eq(chunks[0].start_line, 2);
    expect_eq(chunks[n - 1].end_line, 4);

    // 2. Two short prose paragraphs under the same heading merge when below min_chars
    const short_doc = "# Notes\nShort A.\n\nShort B.";
    const merge_cfg = MdChunkConfig{
        max_chars: 64,
        min_chars: 32,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };
    const n_merged = chunkMarkdownStructured(short_doc, merge_cfg, chunks);
    expect_eq(n_merged, 1);
    expect_eq(chunks[0].start_line, 2);
    expect_eq(chunks[0].end_line, 4);
    expect_eq(std.mem.eql(u8, chunks[0].slice(short_doc), "Short A.\n\nShort B."), true);
}

fn testEmptyAndTruncationBoundaryGuards()! {
    const cfg = MdChunkConfig.default();
    let chunks_buf: [4]MdChunkSpan = [MdChunkSpan{}; 4];
    let chunks: []MdChunkSpan = chunks_buf[0..];

    // Empty, blank-only, heading-only & zero max_chars
    expect_eq(chunkMarkdownStructured("", cfg, chunks), 0);
    expect_eq(chunkMarkdownStructured("   \n\t\n\r\n", cfg, chunks), 0);
    expect_eq(chunkMarkdownStructured("# Title Only\n## Sub Only\n", cfg, chunks), 0);
    const zero_cfg = MdChunkConfig{
        max_chars: 0,
        min_chars: 0,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };
    expect_eq(chunkMarkdownStructured("# A\nhello", zero_cfg, chunks), 0);

    // Truncation safety: 4 chunks total, tested with 0-cap and 2-cap output slices
    const multi_doc = "# A\nPara 1.\n\n# B\nPara 2.\n\n# C\nPara 3.\n\n# D\nPara 4.";
    let zero_buf: [0]MdChunkSpan = undefined;
    let zero_out: []MdChunkSpan = zero_buf[0..];
    expect_eq(chunkMarkdownStructured(multi_doc, cfg, zero_out), 4);

    let small_buf: [2]MdChunkSpan = [MdChunkSpan{}; 2];
    let small_out: []MdChunkSpan = small_buf[0..];
    expect_eq(chunkMarkdownStructured(multi_doc, cfg, small_out), 4);
    expect_eq(std.mem.eql(u8, small_out[0].slice(multi_doc), "Para 1."), true);
    expect_eq(std.mem.eql(u8, small_out[1].slice(multi_doc), "Para 2."), true);
}

fn main() {
    test("1. text markdown_chunk: 6-level heading breadcrumb hierarchy & formatting", () => {
        testHeadingBreadcrumbHierarchy()?;
    });
    test("2. text markdown_chunk: fenced code block integrity & '#' comment immunity", () => {
        testCodeBlockFenceAndHashImmunity()?;
    });
    test("3. text markdown_chunk: GFM table integrity, list detection & split toggle", () => {
        testTableIntegrityAndListSection()?;
    });
    test("4. text markdown_chunk: UTF-8 safe long prose split, short merge & line numbers", () => {
        testLongProseSplitMergeAndLineNumbers()?;
    });
    test("5. text markdown_chunk: empty/heading-only docs, zero max_chars & truncation safety", () => {
        testEmptyAndTruncationBoundaryGuards()?;
    });
}
