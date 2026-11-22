// Source: cp/cap/test/test_bibtex_production_deep.cp (Production deep tests for cap:bibtex)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BibEntry,
    BibField,
    BibEntryIter,
    BibFieldIter,
    findBibField,
    formatBibField,
} from "cap:doc";

const SAMPLE_BIB: []const u8 =
    "% Bibliography database\n" ++
    "@preamble{\"\\newcommand{\\noop}[1]{}\"}\n" ++
    "@comment{This entire block is ignored}\n" ++
    "@string{JACM = \"Journal of the ACM\"}\n" ++
    "@article{knuth1984,\n" ++
    "  author = {Donald E. {Knuth}},\n" ++
    "  Title = \"Literate {Programming}\",\n" ++
    "  journal = {The Computer Journal},\n" ++
    "  year = 1984,\n" ++
    "  pages = {97--111},\n" ++
    "}\n" ++
    "@inproceedings(vaswani2017,\n" ++
    "  author = {Ashish Vaswani and others},\n" ++
    "  title = {Attention Is All You Need},\n" ++
    "  year = 2017\n" ++
    ")\n";

test("1. BibEntryIter skips comments/preamble/string and parses both {...} and (...) entries", () => {
    let it = BibEntryIter.init(SAMPLE_BIB);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.entry_type, "article"), true);
    expect_eq(std.mem.eql(u8, e1.cite_key, "knuth1984"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.entry_type, "inproceedings"), true);
    expect_eq(std.mem.eql(u8, e2.cite_key, "vaswani2017"), true);

    expect_eq(it.next() == null, true);
});

test("2. BibFieldIter parses braced (with nested braces), quoted, and bare numeric fields", () => {
    let it = BibEntryIter.init(SAMPLE_BIB);
    const e1 = it.next() ?? unreachable;

    let fit = BibFieldIter.init(e1.body);

    const f0 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f0.name, "author"), true);
    expect_eq(std.mem.eql(u8, f0.value, "Donald E. {Knuth}"), true);

    const f1 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f1.name, "Title"), true);
    expect_eq(std.mem.eql(u8, f1.value, "Literate {Programming}"), true);

    const f2 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f2.name, "journal"), true);
    expect_eq(std.mem.eql(u8, f2.value, "The Computer Journal"), true);

    const f3 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f3.name, "year"), true);
    expect_eq(std.mem.eql(u8, f3.value, "1984"), true);

    const f4 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f4.name, "pages"), true);
    expect_eq(std.mem.eql(u8, f4.value, "97--111"), true);

    expect_eq(fit.next() == null, true);
});

test("3. findBibField performs ASCII case-insensitive lookup across entries", () => {
    let it = BibEntryIter.init(SAMPLE_BIB);
    const e1 = it.next() ?? unreachable;
    const e2 = it.next() ?? unreachable;

    expect_eq(std.mem.eql(u8, findBibField(e1, "TITLE") ?? "", "Literate {Programming}"), true);
    expect_eq(std.mem.eql(u8, findBibField(e1, "title") ?? "", "Literate {Programming}"), true);
    expect_eq(std.mem.eql(u8, findBibField(e1, "YEAR") ?? "", "1984"), true);
    expect_eq(findBibField(e1, "doi") == null, true);

    expect_eq(std.mem.eql(u8, findBibField(e2, "Year") ?? "", "2017"), true);
    expect_eq(std.mem.eql(u8, findBibField(e2, "AUTHOR") ?? "", "Ashish Vaswani and others"), true);
});

test("4. formatBibField outputs canonical line and enforces buffer bounds", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const line = formatBibField("title", "CP Systems {Design}", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, line, "  title = {CP Systems {Design}},\n"), true);

    // Exact buffer length fit
    const exact_len = line.len;
    const exact = formatBibField("title", "CP Systems {Design}", buf[0..exact_len]) ?? unreachable;
    expect_eq(std.mem.eql(u8, exact, line), true);

    // 1 byte too small -> null
    expect_eq(formatBibField("title", "CP Systems {Design}", buf[0 .. exact_len - 1]) == null, true);
    expect_eq(formatBibField("a", "b", buf[0..0]) == null, true);
});

test("5. Malformed inputs: unclosed entry/braces/quotes, missing cite_key, empty input", () => {
    let it_empty = BibEntryIter.init("");
    expect_eq(it_empty.next() == null, true);

    let it_unclosed = BibEntryIter.init("@article{unclosed, title = {foo}");
    expect_eq(it_unclosed.next() == null, true);

    let it_nokey = BibEntryIter.init("@article{, title = {foo}}");
    expect_eq(it_nokey.next() == null, true);

    let fit_bad_quote = BibFieldIter.init("title = \"unterminated");
    expect_eq(fit_bad_quote.next() == null, true);

    let fit_bad_brace = BibFieldIter.init("title = {unbalanced {inner}");
    expect_eq(fit_bad_brace.next() == null, true);
});
