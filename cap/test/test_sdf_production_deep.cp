// Source: cp/cap/test/test_sdf_production_deep.cp (Production deep tests for cap:sdf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SdfMolHeader,
    SdfAtom,
    SdfBond,
    SdfProperty,
    SdfStats,
    SdfAtomIter,
    SdfBondIter,
    SdfPropIter,
    SdfRecordIter,
    isSdf,
    parseMolHeader,
    findSdfProp,
    summarizeSdf,
} from "cap:sdf";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < eps;
}

// 标准 MDL V2000 苯环（6 碳 6 芳香键）
const BENZENE_MOL: []const u8 =
    "Benzene\n" ++
    "  CP-Chem   11212612002D\n" ++
    "Aromatic ring test\n" ++
    "  6  6  0  0  0  0  0  0  0  0999 V2000\n" ++
    "    1.2124    0.7000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "    1.2124   -0.7000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "    0.0000   -1.4000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "   -1.2124   -0.7000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "   -1.2124    0.7000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "    0.0000    1.4000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0\n" ++
    "  1  2  4  0  0  0  0\n" ++
    "  2  3  4  0  0  0  0\n" ++
    "  3  4  4  0  0  0  0\n" ++
    "  4  5  4  0  0  0  0\n" ++
    "  5  6  4  0  0  0  0\n" ++
    "  6  1  4  0  0  0  0\n" ++
    "M  END\n";

// 多分子 SDF 流（含 CRLF 混用、手性标志、电荷码、多行属性块与 $$$$ 分隔）
const MULTI_MOL_SDF: []const u8 =
    "Water\r\n" ++
    "  -ISIS-  08149511142D\r\n" ++
    "Simple solvent\r\n" ++
    "  3  2  0  0  0  0  0  0  0  0999 V2000\r\n" ++
    "    0.0000    0.0000    0.0000 O   0  0  0  0  0  0\r\n" ++
    "    0.7570    0.5860    0.0000 H   0  0  0  0  0  0\r\n" ++
    "   -0.7570    0.5860    0.0000 H   1  0  0  0  0  0\r\n" ++
    "  1  2  1  0\r\n" ++
    "  1  3  1  0\r\n" ++
    "M  END\r\n" ++
    "> <PUBCHEM_CID>\r\n" ++
    "962\r\n" ++
    "\r\n" ++
    "> <SMILES>\r\n" ++
    "O\r\n" ++
    "\r\n" ++
    "> <SYNONYMS>\r\n" ++
    "Water\r\n" ++
    "Oxidane\r\n" ++
    "H2O\r\n" ++
    "\r\n" ++
    "$$$$\r\n" ++
    "L-Alanine-Zwitterion\n" ++
    "  CP-Chem   11212612003D\n" ++
    "\n" ++
    "  6  5  0  0  1  0  0  0  0  0999 V2000\n" ++
    "    0.0000    0.0000    0.0000 C   0  0  0  0  0  0\n" ++
    "    1.5200    0.0000    0.0000 C   0  0  0  0  0  0\n" ++
    "    2.1000    1.1000    0.0000 O   0  0  0  0  0  0\n" ++
    "    2.1000   -1.1000    0.0000 O   0  5  0  0  0  0\n" ++
    "   -0.5000    1.3500    0.0000 N   0  3  0  0  0  0\n" ++
    "   -0.5000   -0.7500    1.1000 C   0  0  0  0  0  0\n" ++
    "  1  2  1  0\n" ++
    "  2  3  2  0\n" ++
    "  2  4  1  0\n" ++
    "  1  5  1  1\n" ++
    "  1  6  1  6\n" ++
    "M  END\n" ++
    "> <PUBCHEM_CID>\n" ++
    "5950\n" ++
    "\n" ++
    "> <SMILES>\n" ++
    "C[C@@H](C(=O)[O-])[NH3+]\n" ++
    "\n" ++
    "$$$$\n" ++
    "Aspirin-Core\n" ++
    "  CP-Chem   11212612002D\n" ++
    "Acetylsalicylic acid fragment\n" ++
    "  4  3  0  0  0  0  0  0  0  0999 V2000\n" ++
    "    0.0000    0.0000    0.0000 C   0  0\n" ++
    "    1.4000    0.0000    0.0000 C   0  0\n" ++
    "    2.1000    1.2000    0.0000 O   0  0\n" ++
    "    2.1000   -1.2000    0.0000 O   0  0\n" ++
    "  1  2  1  0\n" ++
    "  2  3  2  0\n" ++
    "  2  4  1  0\n" ++
    "M  END\n" ++
    "> <PUBCHEM_CID>\n" ++
    "2244\n" ++
    "\n" ++
    "$$$$\n";

test("1. Benzene V2000 Molfile header, 6 aromatic atoms, and 6 bonds decoding", () => {
    expect_eq(isSdf(BENZENE_MOL), true);
    const hdr = parseMolHeader(BENZENE_MOL) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.title, "Benzene"), true);
    expect_eq(std.mem.eql(u8, hdr.program_line, "  CP-Chem   11212612002D"), true);
    expect_eq(std.mem.eql(u8, hdr.comment, "Aromatic ring test"), true);
    expect_eq(hdr.atom_count, 6);
    expect_eq(hdr.bond_count, 6);
    expect_eq(hdr.is_chiral, false);
    expect_eq(std.mem.eql(u8, hdr.version, "V2000"), true);

    let ait = SdfAtomIter.init(BENZENE_MOL, hdr);
    const a1 = ait.next() ?? unreachable;
    expect_eq(approxEq(a1.x, 1.2124, 0.0001), true);
    expect_eq(approxEq(a1.y, 0.7000, 0.0001), true);
    expect_eq(approxEq(a1.z, 0.0000, 0.0001), true);
    expect_eq(std.mem.eql(u8, a1.symbol, "C"), true);
    expect_eq(a1.mass_diff, 0);
    expect_eq(a1.charge_code, 0);
    expect_eq(a1.formalCharge(), 0);

    let count: u32 = 1;
    while (atom := ait.next()) {
        expect_eq(std.mem.eql(u8, atom.symbol, "C"), true);
        count += 1;
    }
    expect_eq(count, 6);
    expect_eq(ait.malformed, false);

    let bit = SdfBondIter.init(BENZENE_MOL, hdr);
    let bcount: u32 = 0;
    while (b := bit.next()) {
        expect_eq(b.bond_type, 4); // aromatic
        expect_eq(b.stereo, 0);
        bcount += 1;
    }
    expect_eq(bcount, 6);
    expect_eq(bit.malformed, false);
});

test("2. SdfRecordIter multi-molecule $$$$ stream, zwitterion charges, stereo bonds & multiline props", () => {
    let rit = SdfRecordIter.init(MULTI_MOL_SDF);

    // Record 1: Water (CRLF + isotope H mass_diff=1 + multiline SYNONYMS)
    const r1 = rit.next() ?? unreachable;
    const h1 = parseMolHeader(r1) ?? unreachable;
    expect_eq(std.mem.eql(u8, h1.title, "Water"), true);
    expect_eq(h1.atom_count, 3);
    expect_eq(h1.bond_count, 2);

    let a1_it = SdfAtomIter.init(r1, h1);
    * = a1_it.next() ?? unreachable; // O
    * = a1_it.next() ?? unreachable; // H
    const d_atom = a1_it.next() ?? unreachable; // Deuterium (mass_diff = +1)
    expect_eq(std.mem.eql(u8, d_atom.symbol, "H"), true);
    expect_eq(d_atom.mass_diff, 1);

    const cid1 = findSdfProp(r1, "PUBCHEM_CID") ?? unreachable;
    expect_eq(std.mem.eql(u8, cid1, "962"), true);
    const smiles1 = findSdfProp(r1, "SMILES") ?? unreachable;
    expect_eq(std.mem.eql(u8, smiles1, "O"), true);
    const syn1 = findSdfProp(r1, "SYNONYMS") ?? unreachable;
    expect_eq(std.mem.eql(u8, syn1, "Water\r\nOxidane\r\nH2O"), true);
    expect_eq(findSdfProp(r1, "NON_EXISTENT") == null, true);

    // Record 2: L-Alanine-Zwitterion (chiral=true, O- charge_code=5, N+ charge_code=3, stereo 1 & 6)
    const r2 = rit.next() ?? unreachable;
    const h2 = parseMolHeader(r2) ?? unreachable;
    expect_eq(std.mem.eql(u8, h2.title, "L-Alanine-Zwitterion"), true);
    expect_eq(h2.is_chiral, true);

    let a2_it = SdfAtomIter.init(r2, h2);
    * = a2_it.next() ?? unreachable; // 1 C
    * = a2_it.next() ?? unreachable; // 2 C
    * = a2_it.next() ?? unreachable; // 3 O
    const o_minus = a2_it.next() ?? unreachable; // 4 O-
    expect_eq(o_minus.charge_code, 5);
    expect_eq(o_minus.formalCharge(), -1);
    const n_plus = a2_it.next() ?? unreachable; // 5 N+
    expect_eq(n_plus.charge_code, 3);
    expect_eq(n_plus.formalCharge(), 1);

    let b2_it = SdfBondIter.init(r2, h2);
    * = b2_it.next() ?? unreachable; // 1-2 single
    const b_dbl = b2_it.next() ?? unreachable; // 2-3 double
    expect_eq(b_dbl.bond_type, 2);
    * = b2_it.next() ?? unreachable; // 2-4 single
    const b_up = b2_it.next() ?? unreachable; // 1-5 stereo up
    expect_eq(b_up.stereo, 1);
    const b_down = b2_it.next() ?? unreachable; // 1-6 stereo down
    expect_eq(b_down.stereo, 6);

    let p2_it = SdfPropIter.init(r2);
    const p2_0 = p2_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2_0.name, "PUBCHEM_CID"), true);
    expect_eq(std.mem.eql(u8, p2_0.value, "5950"), true);
    const p2_1 = p2_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2_1.name, "SMILES"), true);
    expect_eq(std.mem.eql(u8, p2_1.value, "C[C@@H](C(=O)[O-])[NH3+]"), true);
    expect_eq(p2_it.next() == null, true);

    // Record 3: Aspirin-Core
    const r3 = rit.next() ?? unreachable;
    const h3 = parseMolHeader(r3) ?? unreachable;
    expect_eq(std.mem.eql(u8, h3.title, "Aspirin-Core"), true);
    const cid3 = findSdfProp(r3, "PUBCHEM_CID") ?? unreachable;
    expect_eq(std.mem.eql(u8, cid3, "2244"), true);

    expect_eq(rit.next() == null, true);
});

test("3. SdfAtom.formalCharge full V2000 charge_code 0..7 mapping", () => {
    const expected: [9]i8 = [0, 3, 2, 1, 0, -1, -2, -3, 0];
    let code: u8 = 0;
    while (code <= 8) {
        const atom = SdfAtom{
            x: 0.0,
            y: 0.0,
            z: 0.0,
            symbol: "C",
            mass_diff: 0,
            charge_code: code,
        };
        expect_eq(atom.formalCharge(), expected[usize(code)]);
        code += 1;
    }
});

test("4. summarizeSdf aggregates molecule_count, total_atoms, total_bonds, and chiral_count", () => {
    const stats = summarizeSdf(MULTI_MOL_SDF);
    expect_eq(stats.molecule_count, 3);
    expect_eq(stats.total_atoms, 3 + 6 + 4);
    expect_eq(stats.total_bonds, 2 + 5 + 3);
    expect_eq(stats.chiral_count, 1);

    const empty_stats = summarizeSdf("");
    expect_eq(empty_stats.molecule_count, 0);
    expect_eq(empty_stats.total_atoms, 0);
    expect_eq(empty_stats.total_bonds, 0);
    expect_eq(empty_stats.chiral_count, 0);
});

test("5. Resilience against non-V2000 (V3000), truncated atom/bond blocks, and out-of-bounds bond indices", () => {
    // 非 V2000 (V3000) 拒绝
    const v3000_mol =
        "V3000Test\n" ++
        "  CP\n" ++
        "\n" ++
        "  0  0  0     0  0            999 V3000\n" ++
        "M  END\n";
    expect_eq(parseMolHeader(v3000_mol) == null, true);
    expect_eq(isSdf(v3000_mol), false);

    // 头部不足 4 行
    expect_eq(parseMolHeader("OnlyOneLine\n") == null, true);
    expect_eq(isSdf(""), false);

    // 截断原子块（声明 3 个原子但只有 1 行）
    const trunc_atoms =
        "TruncAtoms\n" ++
        "  CP\n" ++
        "\n" ++
        "  3  0  0  0  0  0  0  0  0  0999 V2000\n" ++
        "    0.0000    0.0000    0.0000 C   0  0\n";
    const h_ta = parseMolHeader(trunc_atoms) ?? unreachable;
    let ait = SdfAtomIter.init(trunc_atoms, h_ta);
    * = ait.next() ?? unreachable;
    expect_eq(ait.next() == null, true);
    expect_eq(ait.malformed, true);

    // 截断键块（声明 2 条键但只有 1 行）
    const trunc_bonds =
        "TruncBonds\n" ++
        "  CP\n" ++
        "\n" ++
        "  2  2  0  0  0  0  0  0  0  0999 V2000\n" ++
        "    0.0000    0.0000    0.0000 C   0  0\n" ++
        "    1.0000    0.0000    0.0000 O   0  0\n" ++
        "  1  2  1  0\n";
    const h_tb = parseMolHeader(trunc_bonds) ?? unreachable;
    let bit_tb = SdfBondIter.init(trunc_bonds, h_tb);
    * = bit_tb.next() ?? unreachable;
    expect_eq(bit_tb.next() == null, true);
    expect_eq(bit_tb.malformed, true);

    // 越界键原子索引（atom2 = 5 > atom_count = 2）与 0 索引
    const oob_bond =
        "OobBond\n" ++
        "  CP\n" ++
        "\n" ++
        "  2  1  0  0  0  0  0  0  0  0999 V2000\n" ++
        "    0.0000    0.0000    0.0000 C   0  0\n" ++
        "    1.0000    0.0000    0.0000 O   0  0\n" ++
        "  1  5  1  0\n";
    const h_oob = parseMolHeader(oob_bond) ?? unreachable;
    let bit_oob = SdfBondIter.init(oob_bond, h_oob);
    expect_eq(bit_oob.next() == null, true);
    expect_eq(bit_oob.malformed, true);

    const zero_bond =
        "ZeroBond\n" ++
        "  CP\n" ++
        "\n" ++
        "  2  1  0  0  0  0  0  0  0  0999 V2000\n" ++
        "    0.0000    0.0000    0.0000 C   0  0\n" ++
        "    1.0000    0.0000    0.0000 O   0  0\n" ++
        "  0  2  1  0\n";
    const h_zero = parseMolHeader(zero_bond) ?? unreachable;
    let bit_zero = SdfBondIter.init(zero_bond, h_zero);
    expect_eq(bit_zero.next() == null, true);
    expect_eq(bit_zero.malformed, true);
});
