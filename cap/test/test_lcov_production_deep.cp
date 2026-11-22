// Source: cp/cap/test/test_lcov_production_deep.cp (Production deep tests for cap:lcov)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LcovRecordKind,
    LcovLineHit,
    LcovBranchHit,
    LcovFnDef,
    LcovFnHit,
    LcovRecord,
    LcovFileSummary,
    LcovTotals,
    LcovRecordIter,
    LcovFileIter,
    summarizeLcov,
} from "cap:binfmt";

test("1. multi-file LCOV .info report parsing and coverage percentage calculation", () => {
    const report =
        "TN:unit_suite\nSF:src/math.cp\nFN:10,add\nFN:20,sub\nFNDA:5,add\nFNDA:0,sub\nFNF:2\nFNH:1\nDA:11,5\nDA:12,5\nDA:21,0\nDA:22,0\nLF:4\nLH:2\nBRDA:11,0,0,5\nBRDA:11,0,1,0\nBRF:2\nBRH:1\nend_of_record\nTN:unit_suite\nSF:src/io.cp\nFN:1,read\nFNDA:3,read\nFNF:1\nFNH:1\nDA:2,3\nDA:3,3\nLF:2\nLH:2\nBRF:0\nBRH:0\nend_of_record\n";

    let fit = LcovFileIter.init(report);

    const f0 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f0.test_name, "unit_suite"), true);
    expect_eq(std.mem.eql(u8, f0.source_file, "src/math.cp"), true);
    expect_eq(f0.lines_found, 4);
    expect_eq(f0.lines_hit, 2);
    expect_eq(f0.lineCoveragePct(), 50.0);
    expect_eq(f0.branches_found, 2);
    expect_eq(f0.branches_hit, 1);
    expect_eq(f0.branchCoveragePct(), 50.0);
    expect_eq(f0.functions_found, 2);
    expect_eq(f0.functions_hit, 1);
    expect_eq(f0.fnCoveragePct(), 50.0);

    const f1 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f1.source_file, "src/io.cp"), true);
    expect_eq(f1.lineCoveragePct(), 100.0);
    expect_eq(f1.fnCoveragePct(), 100.0);
    expect_eq(f1.branchCoveragePct(), 0.0);

    expect_eq(fit.next() == null, true);

    const totals = summarizeLcov(report);
    expect_eq(totals.file_count, 2);
    expect_eq(totals.lines_found, 6);
    expect_eq(totals.lines_hit, 4);
    expect_eq(totals.branches_found, 2);
    expect_eq(totals.branches_hit, 1);
    expect_eq(totals.functions_found, 3);
    expect_eq(totals.functions_hit, 2);
});

test("2. DA with MD5 checksum and BRDA with '-' unexecuted branches decode accurately", () => {
    const text =
        "DA:42,12,d41d8cd98f00b204e9800998ecf8427e\nBRDA:42,1,0,-\nBRDA:42,1,1,7\n";

    let it = LcovRecordIter.init(text);

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.kind == LcovRecordKind.line_data, true);
    expect_eq(r0.line_hit.line, 42);
    expect_eq(r0.line_hit.hits, 12);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.kind == LcovRecordKind.branch_data, true);
    expect_eq(r1.branch_hit.line, 42);
    expect_eq(r1.branch_hit.block, 1);
    expect_eq(r1.branch_hit.branch, 0);
    expect_eq(r1.branch_hit.executed, false);
    expect_eq(r1.branch_hit.taken, 0);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.kind == LcovRecordKind.branch_data, true);
    expect_eq(r2.branch_hit.executed, true);
    expect_eq(r2.branch_hit.taken, 7);

    expect_eq(it.next() == null, true);
});

test("3. missing LF/LH/BRF/BRH/FNF/FNH automatically falls back to detail line counts", () => {
    const no_summary =
        "SF:src/fallback.cp\nFN:1,alpha\nFN:10,beta\nFNDA:4,alpha\nFNDA:0,beta\nDA:2,4\nDA:3,0\nDA:4,2\nDA:11,0\nBRDA:2,0,0,4\nBRDA:2,0,1,-\nend_of_record\n";

    let fit = LcovFileIter.init(no_summary);
    const f = fit.next() ?? unreachable;
    expect_eq(f.lines_found, 4);
    expect_eq(f.lines_hit, 2);
    expect_eq(f.lineCoveragePct(), 50.0);
    expect_eq(f.branches_found, 2);
    expect_eq(f.branches_hit, 1);
    expect_eq(f.branchCoveragePct(), 50.0);
    expect_eq(f.functions_found, 2);
    expect_eq(f.functions_hit, 1);
    expect_eq(f.fnCoveragePct(), 50.0);
});

test("4. CRLF line endings and function names containing commas or C++ template syntax", () => {
    const crlf =
        "\r\nSF:src/tpl.cpp\r\nFN:55,std::map<int, std::string>::insert(int, char const*)\r\nFNDA:9,std::map<int, std::string>::insert(int, char const*)\r\nend_of_record\r\n";

    let it = LcovRecordIter.init(crlf);
    const sf = it.next() ?? unreachable;
    expect_eq(sf.kind == LcovRecordKind.source_file, true);
    expect_eq(std.mem.eql(u8, sf.text, "src/tpl.cpp"), true);

    const fn_def = it.next() ?? unreachable;
    expect_eq(fn_def.kind == LcovRecordKind.fn_def, true);
    expect_eq(fn_def.fn_def.line, 55);
    expect_eq(
        std.mem.eql(u8, fn_def.fn_def.name, "std::map<int, std::string>::insert(int, char const*)"),
        true,
    );

    const fn_hit = it.next() ?? unreachable;
    expect_eq(fn_hit.kind == LcovRecordKind.fn_data, true);
    expect_eq(fn_hit.fn_hit.hits, 9);
    expect_eq(
        std.mem.eql(u8, fn_hit.fn_hit.name, "std::map<int, std::string>::insert(int, char const*)"),
        true,
    );

    const eor = it.next() ?? unreachable;
    expect_eq(eor.kind == LcovRecordKind.end_of_record, true);
});

test("5. empty input and malformed numeric lines degrade safely without panicking", () => {
    const empty_totals = summarizeLcov("");
    expect_eq(empty_totals.file_count, 0);
    expect_eq(empty_totals.lineCoveragePct(), 0.0);

    const malformed =
        "DA:abc,123\nBRDA:1,2,3,notnum\nLF:-5\nUNKNOWN_LINE_NO_COLON\nSF:src/ok.cp\nDA:10,1\nend_of_record\n";

    let it = LcovRecordIter.init(malformed);
    expect_eq((it.next() ?? unreachable).kind == LcovRecordKind.unknown, true);
    expect_eq((it.next() ?? unreachable).kind == LcovRecordKind.unknown, true);
    expect_eq((it.next() ?? unreachable).kind == LcovRecordKind.unknown, true);
    expect_eq((it.next() ?? unreachable).kind == LcovRecordKind.unknown, true);
    expect_eq((it.next() ?? unreachable).kind == LcovRecordKind.source_file, true);

    const totals = summarizeLcov(malformed);
    expect_eq(totals.file_count, 1);
    expect_eq(totals.lines_found, 1);
    expect_eq(totals.lines_hit, 1);
    expect_eq(totals.lineCoveragePct(), 100.0);
});
