// Source: cp/cap/test/test_ris_production_deep.cp (Production deep tests for cap:ris)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RisTag,
    RisRecord,
    RisStats,
    RisTagIter,
    RisIter,
    isRis,
    summarizeRis,
} from "cap:ris";

// 含 UTF-8 BOM、CRLF/LF 混用、JOUR + BOOK + CONF 三条完整文献、多作者、多关键词与多行续行摘要
const SAMPLE_RIS_CORPUS: []const u8 =
    "\xEF\xBB\xBF\r\n" ++
    "TY  - JOUR\r\n" ++
    "TI  - A Mathematical Theory of Communication\r\n" ++
    "AU  - Shannon, Claude E.\r\n" ++
    "AU  - Weaver, Warren\r\n" ++
    "JO  - Bell System Technical Journal\r\n" ++
    "PY  - 1948\r\n" ++
    "VL  - 27\r\n" ++
    "IS  - 3\r\n" ++
    "SP  - 379\r\n" ++
    "EP  - 423\r\n" ++
    "DO  - 10.1002/j.1538-7305.1948.tb01338.x\r\n" ++
    "UR  - https://doi.org/10.1002/j.1538-7305.1948.tb01338.x\r\n" ++
    "SN  - 0005-8580\r\n" ++
    "KW  - information theory\r\n" ++
    "KW  - entropy\r\n" ++
    "AB  - The fundamental problem of communication is that of\r\n" ++
    "reproducing at one point either exactly or approximately\r\n" ++
    "a message selected at another point.\r\n" ++
    "ER  -\r\n" ++
    "\n" ++
    "TY  - BOOK\n" ++
    "T1  - The Art of Computer Programming\n" ++
    "A1  - Knuth, Donald E.\n" ++
    "Y1  - 1968\n" ++
    "PB  - Addison-Wesley\n" ++
    "SN  - 978-0201896831\n" ++
    "KW  - algorithms\n" ++
    "ER  - \n" ++
    "\n" ++
    "TY  - CONF\n" ++
    "TI  - Attention Is All You Need\n" ++
    "AU  - Vaswani, Ashish\n" ++
    "AU  - Shazeer, Noam\n" ++
    "AU  - Parmar, Niki\n" ++
    "T2  - NeurIPS\n" ++
    "PY  - 2017\n" ++
    "DO  - 10.48550/arXiv.1706.03762\n" ++
    "KW  - transformer\n" ++
    "KW  - self-attention\n" ++
    "ER  -\n";

test("1. Multi-record JOUR/BOOK/CONF iteration with full bibliographic fields", () => {
    expect_eq(isRis(SAMPLE_RIS_CORPUS), true);

    let it = RisIter.init(SAMPLE_RIS_CORPUS);

    // Record 1: JOUR
    const r1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.ref_type, "JOUR"), true);
    expect_eq(std.mem.eql(u8, r1.title, "A Mathematical Theory of Communication"), true);
    expect_eq(std.mem.eql(u8, r1.first_author, "Shannon, Claude E."), true);
    expect_eq(r1.author_count, 2);
    expect_eq(std.mem.eql(u8, r1.journal, "Bell System Technical Journal"), true);
    expect_eq(std.mem.eql(u8, r1.year, "1948"), true);
    expect_eq(std.mem.eql(u8, r1.volume, "27"), true);
    expect_eq(std.mem.eql(u8, r1.issue, "3"), true);
    expect_eq(std.mem.eql(u8, r1.start_page, "379"), true);
    expect_eq(std.mem.eql(u8, r1.end_page, "423"), true);
    expect_eq(std.mem.eql(u8, r1.doi, "10.1002/j.1538-7305.1948.tb01338.x"), true);
    expect_eq(std.mem.eql(u8, r1.url, "https://doi.org/10.1002/j.1538-7305.1948.tb01338.x"), true);

    // Record 2: BOOK (uses T1, A1, Y1 aliases)
    const r2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r2.ref_type, "BOOK"), true);
    expect_eq(std.mem.eql(u8, r2.title, "The Art of Computer Programming"), true);
    expect_eq(std.mem.eql(u8, r2.first_author, "Knuth, Donald E."), true);
    expect_eq(r2.author_count, 1);
    expect_eq(std.mem.eql(u8, r2.year, "1968"), true);
    expect_eq(r2.doi.len, 0);

    // Record 3: CONF (uses T2 for venue)
    const r3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r3.ref_type, "CONF"), true);
    expect_eq(std.mem.eql(u8, r3.journal, "NeurIPS"), true);
    expect_eq(r3.author_count, 3);
    expect_eq(std.mem.eql(u8, r3.doi, "10.48550/arXiv.1706.03762"), true);

    expect_eq(it.next() == null, true);
});

test("2. Multi-line continuation AB abstract slicing and RisTagIter traversal", () => {
    let it = RisIter.init(SAMPLE_RIS_CORPUS);
    const r1 = it.next() ?? unreachable;

    const expected_ab =
        "The fundamental problem of communication is that of\r\n" ++
        "reproducing at one point either exactly or approximately\r\n" ++
        "a message selected at another point.";
    expect_eq(std.mem.eql(u8, r1.abstract_text, expected_ab), true);

    let tit = r1.tags();
    let kw_count: u32 = 0;
    let saw_er = false;
    while (tag := tit.next()) {
        if (tag.isCode("KW")) {
            kw_count += 1;
        }
        if (tag.isCode("ER")) {
            saw_er = true;
        }
    }
    expect_eq(kw_count, 2);
    expect_eq(saw_er, true);
});

test("3. Custom tag lookup via findTag (SN, PB) and missing tag returns null", () => {
    let it = RisIter.init(SAMPLE_RIS_CORPUS);
    const r1 = it.next() ?? unreachable;
    const issn = r1.findTag("SN") ?? unreachable;
    expect_eq(std.mem.eql(u8, issn, "0005-8580"), true);
    expect_eq(r1.findTag("ZZ") == null, true);

    const r2 = it.next() ?? unreachable;
    const pub_name = r2.findTag("PB") ?? unreachable;
    expect_eq(std.mem.eql(u8, pub_name, "Addison-Wesley"), true);
    const isbn = r2.findTag("SN") ?? unreachable;
    expect_eq(std.mem.eql(u8, isbn, "978-0201896831"), true);
});

test("4. summarizeRis aggregates counts across JOUR/BOOK/CONF, DOIs, authors, and keywords", () => {
    const stats = summarizeRis(SAMPLE_RIS_CORPUS);
    expect_eq(stats.record_count, 3);
    expect_eq(stats.journal_count, 1);
    expect_eq(stats.book_count, 1);
    expect_eq(stats.conf_count, 1);
    expect_eq(stats.with_doi_count, 2);
    expect_eq(stats.total_authors, 2 + 1 + 3);
    expect_eq(stats.total_keywords, 2 + 1 + 2);

    const empty_stats = summarizeRis("");
    expect_eq(empty_stats.record_count, 0);
    expect_eq(empty_stats.total_authors, 0);
    expect_eq(empty_stats.total_keywords, 0);
});

test("5. Resilience against missing ER  -, non-RIS input, and empty/BOM-only buffers", () => {
    expect_eq(isRis(""), false);
    expect_eq(isRis("\xEF\xBB\xBF\n  \n"), false);
    expect_eq(isRis("@article{shannon1948, title={...}}"), false);

    // 缺失 ER  - 的连续记录仍能安全切分不丢失数据
    const missing_er =
        "TY  - ELEC\n" ++
        "TI  - First Record Without ER\n" ++
        "AU  - Doe, Jane\n" ++
        "TY  - RPRT\n" ++
        "TI  - Second Record At EOF\n" ++
        "AU  - Smith, John\n";

    expect_eq(isRis(missing_er), true);
    let it = RisIter.init(missing_er);
    const a = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, a.ref_type, "ELEC"), true);
    expect_eq(std.mem.eql(u8, a.title, "First Record Without ER"), true);
    expect_eq(a.author_count, 1);

    const b = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, b.ref_type, "RPRT"), true);
    expect_eq(std.mem.eql(u8, b.title, "Second Record At EOF"), true);
    expect_eq(b.author_count, 1);

    expect_eq(it.next() == null, true);
});
