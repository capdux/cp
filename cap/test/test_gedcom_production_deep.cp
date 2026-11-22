// Source: cp/cap/test/test_gedcom_production_deep.cp (Production deep tests for cap:gedcom)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GedcomLine,
    GedcomHeader,
    GedcomIndividual,
    GedcomFamily,
    GedcomStats,
    GedcomLineIter,
    GedcomIndiIter,
    GedcomFamIter,
    isGedcom,
    parseGedcomLine,
    parseGedcomHeader,
    summarizeGedcom,
} from "cap:text";

// 完整 GEDCOM 5.5.1 家谱样例（含 UTF-8 BOM、CRLF/LF 混用、HEAD、3x INDI、1x FAM、SOUR、NOTE、TRLR）
const SAMPLE_GEDCOM: []const u8 =
    "\xEF\xBB\xBF" ++
    "0 HEAD\r\n" ++
    "1 GEDC\r\n" ++
    "2 VERS 5.5.1\r\n" ++
    "2 FORM LINEAGE-LINKED\r\n" ++
    "1 CHAR UTF-8\r\n" ++
    "1 SOUR CP-Genealogy\r\n" ++
    "1 LANG English\r\n" ++
    "\r\n" ++
    "0 @I1@ INDI\n" ++
    "1 NAME John /Smith/\n" ++
    "1 SEX M\n" ++
    "1 BIRT\n" ++
    "2 DATE 14 MAY 1920\n" ++
    "2 PLAC London, England\n" ++
    "1 DEAT\n" ++
    "2 DATE 22 NOV 1995\n" ++
    "2 PLAC Boston, MA, USA\n" ++
    "1 FAMS @F1@\n" ++
    "0 @I2@ INDI\n" ++
    "1 NAME Mary /Jones/\n" ++
    "1 SEX F\n" ++
    "1 BIRT\n" ++
    "2 DATE 3 AUG 1923\n" ++
    "2 PLAC Manchester, England\n" ++
    "1 FAMS @F1@\n" ++
    "0 @I3@ INDI\n" ++
    "1 NAME Alice /Smith/\n" ++
    "1 SEX F\n" ++
    "1 BIRT\n" ++
    "2 DATE 10 JAN 1950\n" ++
    "2 PLAC Boston, MA, USA\n" ++
    "1 FAMC @F1@\n" ++
    "0 @F1@ FAM\n" ++
    "1 HUSB @I1@\n" ++
    "1 WIFE @I2@\n" ++
    "1 CHIL @I3@\n" ++
    "1 CHIL @I4@\n" ++
    "1 MARR\n" ++
    "2 DATE 15 JUN 1945\n" ++
    "2 PLAC London, England\n" ++
    "0 @S1@ SOUR\n" ++
    "1 TITL Parish Register 1920\n" ++
    "0 @N1@ NOTE Family archive compiled in 2026\n" ++
    "1 CONT Additional context line\n" ++
    "0 TRLR\n";

test("1. Full GEDCOM 5.5.1 header, INDI (BIRT/DEAT/DATE/PLAC), and FAM (HUSB/WIFE/CHIL/MARR) parsing", () => {
    expect_eq(isGedcom(SAMPLE_GEDCOM), true);

    const hdr = parseGedcomHeader(SAMPLE_GEDCOM) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.gedc_version, "5.5.1"), true);
    expect_eq(std.mem.eql(u8, hdr.charset, "UTF-8"), true);
    expect_eq(std.mem.eql(u8, hdr.source_system, "CP-Genealogy"), true);
    expect_eq(std.mem.eql(u8, hdr.language, "English"), true);

    // 迭代全部个人记录
    let iit = GedcomIndiIter.init(SAMPLE_GEDCOM);
    const p1 = iit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.xref, "@I1@"), true);
    expect_eq(std.mem.eql(u8, p1.name, "John /Smith/"), true);
    expect_eq(std.mem.eql(u8, p1.sex, "M"), true);
    expect_eq(std.mem.eql(u8, p1.birth_date, "14 MAY 1920"), true);
    expect_eq(std.mem.eql(u8, p1.birth_place, "London, England"), true);
    expect_eq(std.mem.eql(u8, p1.death_date, "22 NOV 1995"), true);
    expect_eq(std.mem.eql(u8, p1.death_place, "Boston, MA, USA"), true);
    expect_eq(std.mem.eql(u8, p1.fams_xref, "@F1@"), true);

    const p2 = iit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.xref, "@I2@"), true);
    expect_eq(std.mem.eql(u8, p2.name, "Mary /Jones/"), true);
    expect_eq(p2.death_date.len, 0);

    const p3 = iit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p3.xref, "@I3@"), true);
    expect_eq(std.mem.eql(u8, p3.famc_xref, "@F1@"), true);
    expect_eq(iit.next() == null, true);

    // 迭代全部家庭记录
    let fit = GedcomFamIter.init(SAMPLE_GEDCOM);
    const f1 = fit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, f1.xref, "@F1@"), true);
    expect_eq(std.mem.eql(u8, f1.husb_xref, "@I1@"), true);
    expect_eq(std.mem.eql(u8, f1.wife_xref, "@I2@"), true);
    expect_eq(std.mem.eql(u8, f1.marr_date, "15 JUN 1945"), true);
    expect_eq(f1.child_count, 2);
    expect_eq(fit.next() == null, true);
});

test("2. Classic CR-only (\\r) and GEDCOM 7.0 header compatibility", () => {
    const ged7 = "0 HEAD\r1 GEDC\r2 VERS 7.0\r1 SCHMA\r0 TRLR\r";
    expect_eq(isGedcom(ged7), true);
    const hdr = parseGedcomHeader(ged7) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.gedc_version, "7.0"), true);

    const stats = summarizeGedcom(ged7);
    expect_eq(stats.line_count, 5);
    expect_eq(stats.max_level, 2);
    expect_eq(stats.has_trailer, true);
});

test("3. isPointerValue() detection and GedcomLineIter strict level-jump validation", () => {
    const l_ptr = parseGedcomLine("1 HUSB @I1@") ?? unreachable;
    expect_eq(l_ptr.isPointerValue(), true);

    const l_txt = parseGedcomLine("1 NAME John /Smith/") ?? unreachable;
    expect_eq(l_txt.isPointerValue(), false);

    const l_esc = parseGedcomLine("1 NOTE @@escaped@@") ?? unreachable;
    expect_eq(l_esc.isPointerValue(), false);

    const l_empty = parseGedcomLine("1 BIRT") ?? unreachable;
    expect_eq(l_empty.isPointerValue(), false);
    expect_eq(l_empty.value.len, 0);

    // 层级非法跳跃：0 -> 2（跳过 1）应触发 malformed
    const bad_jump = "0 HEAD\n2 VERS 5.5.1\n0 TRLR\n";
    let it = GedcomLineIter.init(bad_jump);
    const first = it.next() ?? unreachable;
    expect_eq(first.level, 0);
    expect_eq(it.next() == null, true);
    expect_eq(it.malformed, true);
});

test("4. summarizeGedcom aggregates line_count, INDI/FAM/SOUR/NOTE, max_level, and has_trailer", () => {
    const stats = summarizeGedcom(SAMPLE_GEDCOM);
    expect_eq(stats.indi_count, 3);
    expect_eq(stats.fam_count, 1);
    expect_eq(stats.sour_count, 1);
    expect_eq(stats.note_count, 1);
    expect_eq(stats.max_level, 2);
    expect_eq(stats.has_trailer, true);
    expect_eq(stats.line_count > 35, true);

    const empty_stats = summarizeGedcom("");
    expect_eq(empty_stats.line_count, 0);
    expect_eq(empty_stats.has_trailer, false);
});

test("5. Resilience against non-HEAD start, invalid level numbers, missing TAG, and broken xref", () => {
    expect_eq(isGedcom(""), false);
    expect_eq(isGedcom("0 @I1@ INDI\n1 NAME No Head\n"), false);
    expect_eq(parseGedcomHeader("1 NOTHEAD\n") == null, true);

    expect_eq(parseGedcomLine("-1 HEAD") == null, true);         // 负层级
    expect_eq(parseGedcomLine("abc HEAD") == null, true);        // 非数字层级
    expect_eq(parseGedcomLine("0") == null, true);               // 缺失 TAG
    expect_eq(parseGedcomLine("0 @I1@") == null, true);          // 有 xref 但缺 TAG
    expect_eq(parseGedcomLine("0 @I1 INDI") == null, true);      // 未闭合 xref
    expect_eq(parseGedcomLine("0 @@ INDI") == null, true);       // 空 xref
});
