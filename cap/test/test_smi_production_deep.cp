// Source: cp/cap/test/test_smi_production_deep.cp (Production deep tests for cap:smi)

import { test, expect_eq } from "cap:test";
import {
    SmiTokenKind,
    SmiBracketAtom,
    SmiToken,
    SmiStats,
    SmiLexer,
    parseBracketAtom,
    analyzeSmiles,
    isValidSmiles,
} from "cap:text";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Aspirin, caffeine, ethanol, and NaCl multi-fragment SMILES analysis", () => {
    // 阿司匹林 CC(=O)Oc1ccccc1C(=O)O：13 个重原子，6 个芳香碳，1 个环，13 条化学键，最大分支深度 1
    const aspirin = analyzeSmiles("CC(=O)Oc1ccccc1C(=O)O");
    expect_eq(aspirin.valid, true);
    expect_eq(aspirin.atom_count, 13);
    expect_eq(aspirin.aromatic_atom_count, 6);
    expect_eq(aspirin.ring_count, 1);
    expect_eq(aspirin.bond_count, 13);
    expect_eq(aspirin.branch_depth_max, 1);
    expect_eq(aspirin.fragment_count, 1);
    expect_eq(aspirin.net_charge, 0);

    // 咖啡因 CN1C=NC2=C1C(=O)N(C(=O)N2C)C：14 个重原子，2 个环，15 条键，最大分支深度 2
    const caffeine = analyzeSmiles("CN1C=NC2=C1C(=O)N(C(=O)N2C)C");
    expect_eq(caffeine.valid, true);
    expect_eq(caffeine.atom_count, 14);
    expect_eq(caffeine.ring_count, 2);
    expect_eq(caffeine.bond_count, 15);
    expect_eq(caffeine.branch_depth_max, 2);
    expect_eq(caffeine.fragment_count, 1);

    // 乙醇 CCO：3 原子，2 键，0 环
    const ethanol = analyzeSmiles("CCO");
    expect_eq(ethanol.valid, true);
    expect_eq(ethanol.atom_count, 3);
    expect_eq(ethanol.bond_count, 2);
    expect_eq(ethanol.ring_count, 0);

    // 氯化钠 [Na+].[Cl-]：2 个断开片段，净电荷 0，0 条共价键
    const nacl = analyzeSmiles("[Na+].[Cl-]");
    expect_eq(nacl.valid, true);
    expect_eq(nacl.atom_count, 2);
    expect_eq(nacl.bond_count, 0);
    expect_eq(nacl.fragment_count, 2);
    expect_eq(nacl.net_charge, 0);
});

test("2. Bracket atom parsing: isotopes, chirality, H-count, charges, and atom class", () => {
    const ba1 = parseBracketAtom("13C@@H") ?? unreachable;
    expect_eq(ba1.isotope, 13);
    expect_eq(bytesEq(ba1.symbol, "C"), true);
    expect_eq(ba1.is_aromatic, false);
    expect_eq(ba1.chiral_count, 2);
    expect_eq(ba1.h_count, 1);
    expect_eq(ba1.charge, 0);
    expect_eq(ba1.atom_class, 0);

    const ba2 = parseBracketAtom("NH4+:12") ?? unreachable;
    expect_eq(ba2.isotope, 0);
    expect_eq(bytesEq(ba2.symbol, "N"), true);
    expect_eq(ba2.h_count, 4);
    expect_eq(ba2.charge, 1);
    expect_eq(ba2.atom_class, 12);

    const ba3 = parseBracketAtom("[Fe+3]") ?? unreachable;
    expect_eq(bytesEq(ba3.symbol, "Fe"), true);
    expect_eq(ba3.charge, 3);

    const ba4 = parseBracketAtom("O--") ?? unreachable;
    expect_eq(bytesEq(ba4.symbol, "O"), true);
    expect_eq(ba4.charge, -2);

    const ba5 = parseBracketAtom("c@H:7") ?? unreachable;
    expect_eq(bytesEq(ba5.symbol, "c"), true);
    expect_eq(ba5.is_aromatic, true);
    expect_eq(ba5.chiral_count, 1);
    expect_eq(ba5.h_count, 1);
    expect_eq(ba5.atom_class, 7);

    // 手性两性离子分子整体分析：[13C@@H](O)([NH3+])C(=O)[O-]
    const zwitterion = analyzeSmiles("[13C@@H](O)([NH3+])C(=O)[O-]");
    expect_eq(zwitterion.valid, true);
    expect_eq(zwitterion.atom_count, 6);
    expect_eq(zwitterion.chiral_centers, 1);
    expect_eq(zwitterion.net_charge, 0);
});

test("3. Two-digit %10..%99 ring closures and multi-ring balance", () => {
    const ring10 = analyzeSmiles("C%10CCCCC%10");
    expect_eq(ring10.valid, true);
    expect_eq(ring10.atom_count, 6);
    expect_eq(ring10.ring_count, 1);
    expect_eq(ring10.bond_count, 6);

    // 混合单位与双位环号，并复用环号 1（先闭合再重新打开闭合）
    const multi = analyzeSmiles("C1CCC1C1CC%12CCC1C%12");
    expect_eq(multi.valid, true);
    expect_eq(multi.ring_count, 3);

    // 词法层面精确验证 %10 环号数值
    let lex = SmiLexer.init("C%10CC%10");
    const t0 = lex.next() ?? unreachable;
    expect_eq(t0.kind == .atom, true);
    const t1 = lex.next() ?? unreachable;
    expect_eq(t1.kind == .ring_closure, true);
    expect_eq(t1.ring_num, 10);
    expect_eq(bytesEq(t1.text, "%10"), true);
});

test("4. Lexer disambiguation of two-letter halogens Cl / Br vs single-letter C / B", () => {
    let lex = SmiLexer.init("ClC(Br)B");
    const t0 = lex.next() ?? unreachable;
    expect_eq(t0.kind == .atom, true);
    expect_eq(bytesEq(t0.text, "Cl"), true);

    const t1 = lex.next() ?? unreachable;
    expect_eq(t1.kind == .atom, true);
    expect_eq(bytesEq(t1.text, "C"), true);

    const t2 = lex.next() ?? unreachable;
    expect_eq(t2.kind == .branch_open, true);

    const t3 = lex.next() ?? unreachable;
    expect_eq(t3.kind == .atom, true);
    expect_eq(bytesEq(t3.text, "Br"), true);

    const t4 = lex.next() ?? unreachable;
    expect_eq(t4.kind == .branch_close, true);

    const t5 = lex.next() ?? unreachable;
    expect_eq(t5.kind == .atom, true);
    expect_eq(bytesEq(t5.text, "B"), true);

    expect_eq(lex.next() == null, true);
    expect_eq(lex.had_error, false);

    const st = analyzeSmiles("ClC(Br)B");
    expect_eq(st.valid, true);
    expect_eq(st.atom_count, 4);
    expect_eq(st.bond_count, 3);
});

test("5. Malformed SMILES rejection: unclosed rings, mismatched parens, empty branches, dangling bonds, bad chars", () => {
    expect_eq(isValidSmiles(""), false);
    expect_eq(isValidSmiles("C1CCC"), false);         // 未闭合环
    expect_eq(isValidSmiles("C%10CCC"), false);       // 未闭合双位环
    expect_eq(isValidSmiles("C%1CC"), false);         // 非法 % 环号（仅 1 位数字）
    expect_eq(isValidSmiles("CC(=O"), false);         // 缺少右括号
    expect_eq(isValidSmiles("CC=O)"), false);         // 多余右括号
    expect_eq(isValidSmiles("C()C"), false);          // 空分支
    expect_eq(isValidSmiles("=CC"), false);           // 前导悬空键
    expect_eq(isValidSmiles("CC="), false);           // 尾部悬空键
    expect_eq(isValidSmiles("C(=)C"), false);         // 分支内悬空键
    expect_eq(isValidSmiles("C==C"), false);          // 连续双键
    expect_eq(isValidSmiles(".CC"), false);           // 前导断片点
    expect_eq(isValidSmiles("CC."), false);           // 尾部断片点
    expect_eq(isValidSmiles("[Na+"), false);          // 未闭合方括号
    expect_eq(isValidSmiles("[]"), false);            // 空方括号
    expect_eq(isValidSmiles("C Q C"), false);         // 非法字符与空格

    expect_eq(parseBracketAtom("") == null, true);
    expect_eq(parseBracketAtom("123") == null, true);
    expect_eq(parseBracketAtom("C@@@") == null, true);
    expect_eq(parseBracketAtom("C:") == null, true);
});
