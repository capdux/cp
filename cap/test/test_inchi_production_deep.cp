// Source: cp/cap/test/test_inchi_production_deep.cp (Production deep tests for cap:inchi)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    INCHI_PREFIX,
    INCHI_KEY_LEN,
    InchiLayerKind,
    InchiLayer,
    InchiParsed,
    FormulaElement,
    InchiKey,
    InchiLayerIter,
    FormulaIter,
    isInchi,
    parseInchi,
    parseInchiKey,
    isValidInchiKey,
} from "cap:inchi";

const ASPIRIN_INCHI: []const u8 =
    "InChI=1S/C9H8O4/c1-6(10)13-8-5-3-2-4-7(8)9(11)12/h2-5H,1H3,(H,11,12)";

const L_ALANINE_INCHI: []const u8 =
    "InChI=1S/C3H7NO2/c1-2(4)3(5)6/h2H,4H2,1H3,(H,5,6)/t2-/m0/s1";

const CAFFEINE_INCHI: []const u8 =
    "InChI=1S/C8H10N4O2/c1-10-4-9-6-5(10)7(13)12(3)8(14)11(6)2/h4H,1-3H3";

const NON_STD_FULL_INCHI: []const u8 =
    "InChI=1/C4H6O4/c5-3(6)1-2-4(7)8/h1-2H2,(H,5,6)(H,7,8)/q-1/p+1/b2-1+/i1+1/f/h5H";

test("1. Real Aspirin, L-Alanine stereo (/t/m/s), Caffeine, and non-standard InChI=1 parsing", () => {
    expect_eq(std.mem.eql(u8, INCHI_PREFIX, "InChI="), true);
    expect_eq(INCHI_KEY_LEN, 27);

    // Aspirin
    expect_eq(isInchi(ASPIRIN_INCHI), true);
    const asp = parseInchi(ASPIRIN_INCHI) ?? unreachable;
    expect_eq(std.mem.eql(u8, asp.version, "1S"), true);
    expect_eq(asp.is_standard, true);
    expect_eq(std.mem.eql(u8, asp.formula, "C9H8O4"), true);
    expect_eq(std.mem.eql(u8, asp.connections, "1-6(10)13-8-5-3-2-4-7(8)9(11)12"), true);
    expect_eq(std.mem.eql(u8, asp.hydrogens, "2-5H,1H3,(H,11,12)"), true);
    expect_eq(asp.layer_count, 3);

    // L-Alanine with tetrahedral stereo /t, parity /m, type /s
    const ala = parseInchi(L_ALANINE_INCHI) ?? unreachable;
    expect_eq(std.mem.eql(u8, ala.formula, "C3H7NO2"), true);
    expect_eq(std.mem.eql(u8, ala.tetrahedral_stereo, "2-"), true);
    expect_eq(std.mem.eql(u8, ala.stereo_parity, "0"), true);
    expect_eq(std.mem.eql(u8, ala.stereo_type, "1"), true);
    expect_eq(ala.layer_count, 6);

    // Caffeine
    const caf = parseInchi(CAFFEINE_INCHI) ?? unreachable;
    expect_eq(std.mem.eql(u8, caf.formula, "C8H10N4O2"), true);
    expect_eq(caf.layer_count, 3);

    // Non-standard InChI=1 with /q, /p, /b, /i, /f
    const ns = parseInchi(NON_STD_FULL_INCHI) ?? unreachable;
    expect_eq(std.mem.eql(u8, ns.version, "1"), true);
    expect_eq(ns.is_standard, false);
    expect_eq(std.mem.eql(u8, ns.charge, "-1"), true);
    expect_eq(std.mem.eql(u8, ns.protons, "+1"), true);
    expect_eq(std.mem.eql(u8, ns.double_bond_stereo, "2-1+"), true);
    expect_eq(std.mem.eql(u8, ns.isotopic, "1+1"), true);
    // 主层 /h 不被 /f 之后的 /h5H 覆盖
    expect_eq(std.mem.eql(u8, ns.hydrogens, "1-2H2,(H,5,6)(H,7,8)"), true);
});

test("2. FormulaIter decodes Hill formulas (C9H8O4, C2H6O, NaCl) and multi-component 2H2O.NaCl", () => {
    // C9H8O4
    let f1 = FormulaIter.init("C9H8O4");
    const e1 = f1.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.symbol, "C"), true);
    expect_eq(e1.count, 9);
    const e2 = f1.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.symbol, "H"), true);
    expect_eq(e2.count, 8);
    const e3 = f1.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.symbol, "O"), true);
    expect_eq(e3.count, 4);
    expect_eq(f1.next() == null, true);
    expect_eq(f1.malformed, false);

    // C2H6O (implicit count = 1 for O)
    let f2 = FormulaIter.init("C2H6O");
    * = f2.next() ?? unreachable;
    * = f2.next() ?? unreachable;
    const o_el = f2.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, o_el.symbol, "O"), true);
    expect_eq(o_el.count, 1);
    expect_eq(f2.next() == null, true);

    // Multi-component "2H2O.NaCl" -> H:4, O:2, Na:1, Cl:1
    let f3 = FormulaIter.init("2H2O.NaCl");
    const m1 = f3.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.symbol, "H"), true);
    expect_eq(m1.count, 4);
    const m2 = f3.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.symbol, "O"), true);
    expect_eq(m2.count, 2);
    const m3 = f3.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m3.symbol, "Na"), true);
    expect_eq(m3.count, 1);
    const m4 = f3.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m4.symbol, "Cl"), true);
    expect_eq(m4.count, 1);
    expect_eq(f3.next() == null, true);
    expect_eq(f3.malformed, false);

    // Malformed formula
    let fbad = FormulaIter.init("C9?H8");
    * = fbad.next() ?? unreachable;
    expect_eq(fbad.next() == null, true);
    expect_eq(fbad.malformed, true);
});

test("3. parseInchiKey & isValidInchiKey for Standard/Non-standard keys and protonationDelta()", () => {
    // Aspirin Standard InChIKey (neutral 'N' -> 0)
    const asp_key = "BSYNRYMUTXBXSQ-UHFFFAOYSA-N";
    expect_eq(isValidInchiKey(asp_key), true);
    const k1 = parseInchiKey(asp_key) ?? unreachable;
    expect_eq(std.mem.eql(u8, k1.connectivity_block, "BSYNRYMUTXBXSQ"), true);
    expect_eq(std.mem.eql(u8, k1.stereo_block, "UHFFFAOY"), true);
    expect_eq(k1.flag_char, 'S');
    expect_eq(k1.version_char, 'A');
    expect_eq(k1.protonation_char, 'N');
    expect_eq(k1.is_standard, true);
    expect_eq(k1.protonationDelta(), 0);

    // Protonated (+1 -> 'O') & Non-standard ('N')
    const prot_key = "QNAYBMKLOCPYGJ-REOHCLBHNA-O";
    const k2 = parseInchiKey(prot_key) ?? unreachable;
    expect_eq(k2.flag_char, 'N');
    expect_eq(k2.is_standard, false);
    expect_eq(k2.protonation_char, 'O');
    expect_eq(k2.protonationDelta(), 1);

    // Deprotonated (-1 -> 'M')
    const deprot_key = "QNAYBMKLOCPYGJ-REOHCLBHSA-M";
    const k3 = parseInchiKey(deprot_key) ?? unreachable;
    expect_eq(k3.is_standard, true);
    expect_eq(k3.protonation_char, 'M');
    expect_eq(k3.protonationDelta(), -1);
});

test("4. InchiLayerIter traverses every /... layer in order with exact prefix and kind", () => {
    let it = InchiLayerIter.init(L_ALANINE_INCHI);

    const l0 = it.next() ?? unreachable;
    expect_eq(l0.prefix, 0);
    expect_eq(l0.kind == InchiLayerKind.formula, true);
    expect_eq(std.mem.eql(u8, l0.content, "C3H7NO2"), true);

    const l1 = it.next() ?? unreachable;
    expect_eq(l1.prefix, 'c');
    expect_eq(l1.kind == InchiLayerKind.connections, true);
    expect_eq(std.mem.eql(u8, l1.content, "1-2(4)3(5)6"), true);

    const l2 = it.next() ?? unreachable;
    expect_eq(l2.prefix, 'h');
    expect_eq(l2.kind == InchiLayerKind.hydrogens, true);

    const l3 = it.next() ?? unreachable;
    expect_eq(l3.prefix, 't');
    expect_eq(l3.kind == InchiLayerKind.tetrahedral_stereo, true);
    expect_eq(std.mem.eql(u8, l3.content, "2-"), true);

    const l4 = it.next() ?? unreachable;
    expect_eq(l4.prefix, 'm');
    expect_eq(l4.kind == InchiLayerKind.stereo_parity, true);

    const l5 = it.next() ?? unreachable;
    expect_eq(l5.prefix, 's');
    expect_eq(l5.kind == InchiLayerKind.stereo_type, true);

    expect_eq(it.next() == null, true);
});

test("5. Resilience against invalid InChI prefixes, missing formula, whitespace, and malformed InChIKeys", () => {
    // 无效 InChI
    expect_eq(isInchi(""), false);
    expect_eq(isInchi("SMILES=CCO"), false);
    expect_eq(parseInchi("InChI=1S/") == null, true);               // 缺分子式
    expect_eq(parseInchi("InChI=1S//c1-2") == null, true);          // 空分子式
    expect_eq(parseInchi("InChI=1S/c1-2") == null, true);           // 小写开头非分子式
    expect_eq(parseInchi("InChI=1X/CH4") == null, true);            // 非法版本后缀
    expect_eq(parseInchi("InChI=1S/CH4 /h1H4") == null, true);      // 含空格

    // 无效 InChIKey
    expect_eq(isValidInchiKey(""), false);
    expect_eq(isValidInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYSA-"), false);       // 26 字节错长
    expect_eq(isValidInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYSA-NN"), false);     // 28 字节错长
    expect_eq(isValidInchiKey("bsynrymutxbxsq-UHFFFAOYSA-N"), false);      // 非法小写
    expect_eq(isValidInchiKey("BSYNRYMUTXBXSQ_UHFFFAOYSA-N"), false);      // 错分隔符
    expect_eq(isValidInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYXA-N"), false);      // 非法 flag 'X' (仅 S/N)
    expect_eq(isValidInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYSA-1"), false);      // 非法质子化数字
    expect_eq(parseInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYSA-1") == null, true);
});
