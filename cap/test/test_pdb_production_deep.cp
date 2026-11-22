// Source: cp/cap/test/test_pdb_production_deep.cp (Production deep tests for cap:pdb)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PdbHeader,
    PdbAtom,
    PdbConect,
    PdbStats,
    PdbAtomIter,
    PdbConectIter,
    isPdb,
    parsePdbHeader,
    parsePdbAtomLine,
    parsePdbConectLine,
    summarizePdb,
} from "cap:science";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < eps;
}

// 标准 RCSB 1CRN 风格 80 列定宽 PDB 片段（含 CRLF 混用、ATOM、TER、HETATM、CONECT、END 及 END 后尾随数据）
const SAMPLE_1CRN_PDB: []const u8 =
    "HEADER    PLANT PROTEIN                           30-APR-81   1CRN              \r\n" ++
    "TITLE     WATER STRUCTURE OF A HYDROPHOBIC PROTEIN AT ATOMIC RESOLUTION         \r\n" ++
    "TITLE    2 PENTAGON RINGS OF WATER MOLECULES IN CRYSTALS OF CRAMBIN             \n" ++
    "EXPDTA    X-RAY DIFFRACTION                                                     \n" ++
    "ATOM      1  N   THR A   1      17.047  14.099   3.625  1.00 13.79           N  \n" ++
    "ATOM      2  CA  THR A   1      16.967  12.784   4.338  1.00 10.80           C  \r\n" ++
    "ATOM      3  C   THR A   1      15.685  12.755   5.133  1.00  9.19           C  \n" ++
    "ATOM      4  O   THR A   1      15.268  13.825   5.594  1.00  9.85           O  \n" ++
    "TER       5      THR A   1                                                      \n" ++
    "HETATM    6  O   HOH A 101      12.345  10.125   6.250  0.50 24.50           O  \n" ++
    "HETATM    7 ZN    ZN A 201      10.000  11.000   5.000  1.00 15.25          ZN2+\n" ++
    "CONECT    1    2                                                                \n" ++
    "CONECT    2    1    3                                                           \n" ++
    "END                                                                             \n" ++
    "ATOM    999  N   GLY Z 999      99.000  99.000  99.000  1.00  0.00           N  \n";

test("1. Standard 80-col RCSB HEADER/TITLE/EXPDTA/ATOM/HETATM/TER/END parsing", () => {
    expect_eq(isPdb(SAMPLE_1CRN_PDB), true);

    const hdr = parsePdbHeader(SAMPLE_1CRN_PDB) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.classification, "PLANT PROTEIN"), true);
    expect_eq(std.mem.eql(u8, hdr.dep_date, "30-APR-81"), true);
    expect_eq(std.mem.eql(u8, hdr.pdb_id, "1CRN"), true);
    expect_eq(std.mem.eql(u8, hdr.title, "WATER STRUCTURE OF A HYDROPHOBIC PROTEIN AT ATOMIC RESOLUTION"), true);
    expect_eq(std.mem.eql(u8, hdr.expdta, "X-RAY DIFFRACTION"), true);

    let it = PdbAtomIter.init(SAMPLE_1CRN_PDB);

    const a1 = it.next() ?? unreachable;
    expect_eq(a1.is_hetatm, false);
    expect_eq(a1.serial, 1);
    expect_eq(std.mem.eql(u8, a1.name, "N"), true);
    expect_eq(a1.alt_loc, ' ');
    expect_eq(std.mem.eql(u8, a1.res_name, "THR"), true);
    expect_eq(a1.chain_id, 'A');
    expect_eq(a1.res_seq, 1);
    expect_eq(approxEq(a1.x, 17.047, 0.0001), true);
    expect_eq(approxEq(a1.y, 14.099, 0.0001), true);
    expect_eq(approxEq(a1.z, 3.625, 0.0001), true);
    expect_eq(approxEq(a1.occupancy, 1.00, 0.0001), true);
    expect_eq(approxEq(a1.temp_factor, 13.79, 0.0001), true);
    expect_eq(std.mem.eql(u8, a1.element, "N"), true);

    const a2 = it.next() ?? unreachable;
    expect_eq(a2.serial, 2);
    expect_eq(std.mem.eql(u8, a2.name, "CA"), true);

    const a3 = it.next() ?? unreachable;
    expect_eq(a3.serial, 3);
    expect_eq(std.mem.eql(u8, a3.name, "C"), true);

    const a4 = it.next() ?? unreachable;
    expect_eq(a4.serial, 4);
    expect_eq(std.mem.eql(u8, a4.name, "O"), true);

    // TER 被自动跳过，下一条为 HETATM HOH
    const h1 = it.next() ?? unreachable;
    expect_eq(h1.is_hetatm, true);
    expect_eq(h1.serial, 6);
    expect_eq(std.mem.eql(u8, h1.res_name, "HOH"), true);
    expect_eq(h1.res_seq, 101);
    expect_eq(approxEq(h1.occupancy, 0.50, 0.0001), true);
    expect_eq(approxEq(h1.temp_factor, 24.50, 0.0001), true);
    expect_eq(std.mem.eql(u8, h1.element, "O"), true);

    // 含电荷列 79..80 的 ZN2+
    const h2 = it.next() ?? unreachable;
    expect_eq(h2.is_hetatm, true);
    expect_eq(h2.serial, 7);
    expect_eq(std.mem.eql(u8, h2.element, "ZN"), true);
    expect_eq(std.mem.eql(u8, h2.charge, "2+"), true);

    // 遇 END 停止，不读取 END 之后的 ATOM 999
    expect_eq(it.next() == null, true);
    expect_eq(it.next() == null, true);
});

const SAMPLE_NMR_MULTI_MODEL: []const u8 =
    "HEADER    SIGNALING PROTEIN                       12-JAN-20   6NMR              \n" ++
    "EXPDTA    SOLUTION NMR                                                          \n" ++
    "MODEL        1                                                                  \n" ++
    "ATOM      1  CA  ALA A   1       0.000   0.000   0.000  1.00  0.00           C  \n" ++
    "ATOM      2  CA  GLY B   2       2.000   4.000   6.000  1.00  0.00           C  \n" ++
    "ENDMDL                                                                          \n" ++
    "MODEL        2                                                                  \n" ++
    "ATOM      1  CA  ALA A   1      -2.000   2.000   2.000  1.00  0.00           C  \n" ++
    "HETATM    3  O   HOH C 100       4.000  -2.000   0.000  1.00  0.00           O  \n" ++
    "ENDMDL                                                                          \n" ++
    "END                                                                             \n";

test("2. Multi-MODEL NMR structure tracking, chain deduplication, bounding box & centroid", () => {
    const hdr = parsePdbHeader(SAMPLE_NMR_MULTI_MODEL) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.pdb_id, "6NMR"), true);
    expect_eq(std.mem.eql(u8, hdr.expdta, "SOLUTION NMR"), true);

    let it = PdbAtomIter.init(SAMPLE_NMR_MULTI_MODEL);
    const m1_a1 = it.next() ?? unreachable;
    expect_eq(it.model_num, 1);
    expect_eq(m1_a1.model_num, 1);
    expect_eq(m1_a1.chain_id, 'A');

    const m1_a2 = it.next() ?? unreachable;
    expect_eq(it.model_num, 1);
    expect_eq(m1_a2.chain_id, 'B');

    // ENDMDL 不会误触发 END 停止
    const m2_a1 = it.next() ?? unreachable;
    expect_eq(it.model_num, 2);
    expect_eq(m2_a1.model_num, 2);
    expect_eq(m2_a1.chain_id, 'A');

    const m2_h3 = it.next() ?? unreachable;
    expect_eq(it.model_num, 2);
    expect_eq(m2_h3.is_hetatm, true);
    expect_eq(m2_h3.chain_id, 'C');

    expect_eq(it.next() == null, true);

    const stats = summarizePdb(SAMPLE_NMR_MULTI_MODEL);
    expect_eq(stats.atom_count, 3);
    expect_eq(stats.hetatm_count, 1);
    expect_eq(stats.model_count, 2);
    expect_eq(stats.chain_count, 3); // A, B, C
    expect_eq(approxEq(stats.min_x, -2.0, 0.0001), true);
    expect_eq(approxEq(stats.max_x, 4.0, 0.0001), true);
    expect_eq(approxEq(stats.min_y, -2.0, 0.0001), true);
    expect_eq(approxEq(stats.max_y, 4.0, 0.0001), true);
    expect_eq(approxEq(stats.min_z, 0.0, 0.0001), true);
    expect_eq(approxEq(stats.max_z, 6.0, 0.0001), true);
    // sum_x = 0 + 2 - 2 + 4 = 4 -> centroid_x = 1.0
    // sum_y = 0 + 4 + 2 - 2 = 4 -> centroid_y = 1.0
    // sum_z = 0 + 6 + 2 + 0 = 8 -> centroid_z = 2.0
    expect_eq(approxEq(stats.centroid_x, 1.0, 0.0001), true);
    expect_eq(approxEq(stats.centroid_y, 1.0, 0.0001), true);
    expect_eq(approxEq(stats.centroid_z, 2.0, 0.0001), true);
});

test("3. distanceTo 3D Euclidean geometry for peptide bond, hydrogen bond, and 3-4-5 triangle", () => {
    // 典型肽键 C(i)-N(i+1) ~ 1.33 Å
    const line_c = "ATOM     10  C   ALA A   5      10.000  20.000  30.000  1.00 10.00           C  ";
    const line_n = "ATOM     11  N   GLY A   6      11.330  20.000  30.000  1.00 10.00           N  ";
    // 典型主链氢键 O...N ~ 2.90 Å (dx=0, dy=2.90, dz=0)
    const line_o = "ATOM     12  O   VAL A   9      11.330  22.900  30.000  1.00 12.00           O  ";
    // 3-4-5 直角三角形验证多轴 3D 距离 (dx=3, dy=4, dz=0 -> 5.0; dx=0, dy=0, dz=12 -> 13.0)
    const line_p = "ATOM     13  CA  LEU A  10      13.000  24.000  42.000  1.00 12.00           C  ";

    const atom_c = parsePdbAtomLine(line_c) ?? unreachable;
    const atom_n = parsePdbAtomLine(line_n) ?? unreachable;
    const atom_o = parsePdbAtomLine(line_o) ?? unreachable;
    const atom_p = parsePdbAtomLine(line_p) ?? unreachable;

    expect_eq(approxEq(atom_c.distanceTo(atom_n), 1.33, 0.0001), true);
    expect_eq(approxEq(atom_n.distanceTo(atom_o), 2.90, 0.0001), true);
    // sqrt(3^2 + 4^2 + 12^2) = sqrt(9 + 16 + 144) = sqrt(169) = 13.0
    expect_eq(approxEq(atom_c.distanceTo(atom_p), 13.0, 0.0001), true);
    expect_eq(approxEq(atom_c.distanceTo(atom_c), 0.0, 0.000001), true);
});

test("4. Short 54-col ATOM lines without occupancy/tempFactor/element use spec defaults", () => {
    // 精确 54 列（无 occupancy, temp_factor, element, charge）
    const short54 = "ATOM     42  CB AASP B  17      -1.250   2.500  -3.750";
    expect_eq(short54.len, 54);

    const atom = parsePdbAtomLine(short54) ?? unreachable;
    expect_eq(atom.is_hetatm, false);
    expect_eq(atom.serial, 42);
    expect_eq(std.mem.eql(u8, atom.name, "CB"), true);
    expect_eq(atom.alt_loc, 'A');
    expect_eq(std.mem.eql(u8, atom.res_name, "ASP"), true);
    expect_eq(atom.chain_id, 'B');
    expect_eq(atom.res_seq, 17);
    expect_eq(approxEq(atom.x, -1.25, 0.0001), true);
    expect_eq(approxEq(atom.y, 2.50, 0.0001), true);
    expect_eq(approxEq(atom.z, -3.75, 0.0001), true);
    // 缺省值验证
    expect_eq(approxEq(atom.occupancy, 1.0, 0.0001), true);
    expect_eq(approxEq(atom.temp_factor, 0.0, 0.0001), true);
    expect_eq(atom.element.len, 0);
    expect_eq(atom.charge.len, 0);

    // 仅带 occupancy (60 列)，缺少 temp_factor 与 element
    const short60 = "HETATM   99  O   HOH A 300       1.000   2.000   3.000  0.75";
    const h = parsePdbAtomLine(short60) ?? unreachable;
    expect_eq(h.is_hetatm, true);
    expect_eq(approxEq(h.occupancy, 0.75, 0.0001), true);
    expect_eq(approxEq(h.temp_factor, 0.0, 0.0001), true);
    expect_eq(h.element.len, 0);
});

test("5. Malformed lines, truncated ATOM (<54 cols), invalid coordinates, and empty input resilience", () => {
    // 截断行（53 列 < 54）
    const trunc53 = "ATOM     42  CB  ASP B  17      -1.250   2.500  -3.75";
    expect_eq(trunc53.len, 53);
    expect_eq(parsePdbAtomLine(trunc53) == null, true);

    // 非法浮点坐标（NaN / 字母）
    const bad_x = "ATOM      1  N   THR A   1       17.0X  14.099   3.625  1.00 13.79           N  ";
    const bad_y = "ATOM      1  N   THR A   1      17.047     NaN   3.625  1.00 13.79           N  ";
    const bad_z = "ATOM      1  N   THR A   1      17.047  14.099     ---  1.00 13.79           N  ";
    const bad_serial = "ATOM    XYZ  N   THR A   1      17.047  14.099   3.625  1.00 13.79           N  ";
    const bad_occ = "ATOM      1  N   THR A   1      17.047  14.099   3.625  bad! 13.79           N  ";
    const not_atom = "REMARK    1  N   THR A   1      17.047  14.099   3.625  1.00 13.79           N  ";

    expect_eq(parsePdbAtomLine(bad_x) == null, true);
    expect_eq(parsePdbAtomLine(bad_y) == null, true);
    expect_eq(parsePdbAtomLine(bad_z) == null, true);
    expect_eq(parsePdbAtomLine(bad_serial) == null, true);
    expect_eq(parsePdbAtomLine(bad_occ) == null, true);
    expect_eq(parsePdbAtomLine(not_atom) == null, true);

    // 空输入与非 PDB 文本
    expect_eq(isPdb(""), false);
    expect_eq(isPdb("{\"not\": \"pdb\"}"), false);
    expect_eq(parsePdbHeader("") == null, true);
    expect_eq(parsePdbHeader("JUST RANDOM TEXT\n") == null, true);

    const empty_stats = summarizePdb("");
    expect_eq(empty_stats.atom_count, 0);
    expect_eq(empty_stats.hetatm_count, 0);
    expect_eq(empty_stats.model_count, 0);
    expect_eq(empty_stats.chain_count, 0);

    // 混合畸形行与合法行时，迭代器自动跳过畸形行继续解析合法记录
    const mixed =
        "ATOM      1  N   THR A   1       bad_x  14.099   3.625\n" ++
        "ATOM      2  CA  THR A   1      16.967  12.784\n" ++
        "ATOM      3  C   THR A   1       1.000   2.000   3.000\n";
    let it = PdbAtomIter.init(mixed);
    const valid = it.next() ?? unreachable;
    expect_eq(valid.serial, 3);
    expect_eq(it.next() == null, true);
});

test("6. CONECT covalent bond parsing and PdbConectIter traversal", () => {
    let cit = PdbConectIter.init(SAMPLE_1CRN_PDB);

    const c1 = cit.next() ?? unreachable;
    expect_eq(c1.serial, 1);
    expect_eq(c1.bond_count, 1);
    expect_eq(c1.bonds[0], 2);

    const c2 = cit.next() ?? unreachable;
    expect_eq(c2.serial, 2);
    expect_eq(c2.bond_count, 2);
    expect_eq(c2.bonds[0], 1);
    expect_eq(c2.bonds[1], 3);

    expect_eq(cit.next() == null, true);

    // 满 4 键 CONECT 与畸形 CONECT 容错
    const c4 = parsePdbConectLine("CONECT  100  101  102  103  104") ?? unreachable;
    expect_eq(c4.serial, 100);
    expect_eq(c4.bond_count, 4);
    expect_eq(c4.bonds[0], 101);
    expect_eq(c4.bonds[3], 104);

    expect_eq(parsePdbConectLine("CONECT  bad  101") == null, true);
    expect_eq(parsePdbConectLine("CONECT") == null, true);
});
