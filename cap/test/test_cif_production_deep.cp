// Source: cp/cap/test/test_cif_production_deep.cp
// Deep Production Test Suite for cap:cif
// Covers:
//   1. Standard crystal CIF (data_quartz + _cell_length_a + quoted space group + loop_ atoms)
//   2. mmCIF (PDBx) _entry.id + _atom_site.* multi-column loop_ & colIndex lookup
//   3. Column-0 semicolon multiline ;...; text fields & embedded quotes in '...' / "..."
//   4. "." missing & "?" unknown sentinel flags vs quoted '.' / '?' literals
//   5. Non-CIF text, unterminated ;...; multiline block & incomplete loop_ row guards

import { test, expect_eq } from "cap:test";
import {
    CifTokenKind,
    CifToken,
    CifEntry,
    CifStats,
    isCif,
    CifLexer,
    CifPairIter,
    findCifTag,
    CifRow,
    CifRowIter,
    CifLoop,
    CifLoopIter,
    summarizeCif,
} from "cap:cif";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("cif: standard crystal CIF (data_quartz, cell params, quoted space group, loop_ atoms)") {
    const doc =
        "# Quartz crystal structure (IUCr CIF 1.1)\n" ++
        "data_quartz\n" ++
        "_cell_length_a 4.9137\n" ++
        "_cell_length_c 5.4047\n" ++
        "_symmetry_space_group_name_H-M 'P 32 2 1'\n" ++
        "loop_\n" ++
        "_atom_site_label\n" ++
        "_atom_site_type_symbol\n" ++
        "_atom_site_fract_x\n" ++
        "_atom_site_fract_y\n" ++
        "_atom_site_fract_z\n" ++
        "Si1 Si 0.4697 0.0000 0.0000\n" ++
        "O1  O  0.4135 0.2669 0.1191\n";

    expect_eq(isCif(doc), true);

    if (a := findCifTag(doc, "_cell_length_a")) {
        expect_eq(strEq(a, "4.9137"), true);
    } else {
        expect_eq(true, false);
    }

    if (sg := findCifTag(doc, "_SYMMETRY_SPACE_GROUP_NAME_H-M")) {
        expect_eq(strEq(sg, "P 32 2 1"), true);
    } else {
        expect_eq(true, false);
    }

    if (stats := summarizeCif(doc)) {
        expect_eq(stats.data_block_count, 1);
        expect_eq(stats.tag_pair_count, 3);
        expect_eq(stats.loop_count, 1);
        expect_eq(stats.loop_row_count, 2);
    } else {
        expect_eq(true, false);
    }

    // Verify loop_ table columns and row iteration
    let loops = CifLoopIter.init(doc);
    if (lp := loops.next()) {
        expect_eq(lp.colCount(), 5);
        if (x_col := lp.colIndex("_atom_site_fract_x")) {
            expect_eq(x_col, 2);
            let rows = lp.rows();
            if (r1 := rows.next()) {
                if (lbl := r1.get(0)) {
                    expect_eq(strEq(lbl, "Si1"), true);
                } else {
                    expect_eq(true, false);
                }
                if (x1 := r1.get(x_col)) {
                    expect_eq(strEq(x1, "0.4697"), true);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
            if (r2 := rows.next()) {
                if (lbl2 := r2.get(0)) {
                    expect_eq(strEq(lbl2, "O1"), true);
                } else {
                    expect_eq(true, false);
                }
                if (x2 := r2.get(x_col)) {
                    expect_eq(strEq(x2, "0.4135"), true);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
            if (r3 := rows.next()) {
                * = r3;
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("cif: mmCIF (PDBx) _entry.id and _atom_site.* multi-column loop_ with colIndex") {
    const mmcif =
        "DATA_1CRN\n" ++
        "_entry.id 1CRN\n" ++
        "_exptl.method 'X-RAY DIFFRACTION'\n" ++
        "LOOP_\n" ++
        "_atom_site.group_PDB\n" ++
        "_atom_site.id\n" ++
        "_atom_site.type_symbol\n" ++
        "_atom_site.label_comp_id\n" ++
        "_atom_site.Cartn_x\n" ++
        "ATOM 1 N THR 17.047\n" ++
        "ATOM 2 C THR 16.967\n" ++
        "ATOM 3 O THR 15.685\n";

    expect_eq(isCif(mmcif), true);
    if (eid := findCifTag(mmcif, "_entry.id")) {
        expect_eq(strEq(eid, "1CRN"), true);
    } else {
        expect_eq(true, false);
    }

    let loops = CifLoopIter.init(mmcif);
    if (lp := loops.next()) {
        expect_eq(lp.colCount(), 5);
        if (comp_col := lp.colIndex("_atom_site.label_comp_id")) {
            expect_eq(comp_col, 3);
        } else {
            expect_eq(true, false);
        }
        if (x_col := lp.colIndex("_ATOM_SITE.CARTN_X")) {
            expect_eq(x_col, 4);
            let rows = lp.rows();
            let count: u32 = 0;
            while (row := rows.next()) {
                count += 1;
                if (count == 3) {
                    if (v := row.get(x_col)) {
                        expect_eq(strEq(v, "15.685"), true);
                    } else {
                        expect_eq(true, false);
                    }
                }
            }
            expect_eq(count, 3);
        } else {
            expect_eq(true, false);
        }
        if (missing := lp.colIndex("_atom_site.nonexistent")) {
            * = missing;
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("cif: column-0 semicolon multiline ;...; text field & embedded quotes in strings") {
    const doc =
        "data_text\n" ++
        "_struct.title\n" ++
        ";\n" ++
        "Crystal structure of crambin\n" ++
        "at 0.54 A resolution\n" ++
        ";\n" ++
        "_chem_comp.name 'a's b'\n" ++
        "_atom.label \"O2' atom\"\n";

    if (title := findCifTag(doc, "_struct.title")) {
        expect_eq(strEq(title, "Crystal structure of crambin\nat 0.54 A resolution"), true);
    } else {
        expect_eq(true, false);
    }

    if (name := findCifTag(doc, "_chem_comp.name")) {
        expect_eq(strEq(name, "a's b"), true);
    } else {
        expect_eq(true, false);
    }

    if (lbl := findCifTag(doc, "_atom.label")) {
        expect_eq(strEq(lbl, "O2' atom"), true);
    } else {
        expect_eq(true, false);
    }
}

test("cif: '.' missing and '?' unknown sentinels vs quoted '.' and '?' literals") {
    const doc =
        "data_sentinels\n" ++
        "_a.missing .\n" ++
        "_a.unknown ?\n" ++
        "_a.lit_dot '.'\n" ++
        "_a.lit_q   \"?\"\n";

    let pairs = CifPairIter.init(doc);
    let idx: u32 = 0;
    while (entry := pairs.next()) {
        idx += 1;
        if (idx == 1) {
            expect_eq(entry.is_missing, true);
            expect_eq(entry.is_unknown, false);
        } else if (idx == 2) {
            expect_eq(entry.is_missing, false);
            expect_eq(entry.is_unknown, true);
        } else if (idx == 3) {
            expect_eq(entry.is_missing, false);
            expect_eq(strEq(entry.value, "."), true);
        } else if (idx == 4) {
            expect_eq(entry.is_unknown, false);
            expect_eq(strEq(entry.value, "?"), true);
        }
    }
    expect_eq(idx, 4);
    expect_eq(pairs.hasError(), false);
}

test("cif: non-CIF text, unterminated ;...; multiline block & incomplete loop_ row guards") {
    // 1. Non-CIF text
    expect_eq(isCif("# just a comment\nhello world\n"), false);
    if (s := summarizeCif("not a cif file")) {
        * = s;
        expect_eq(true, false);
    }

    // 2. Unterminated multiline semicolon block
    const unterm_semi =
        "data_bad\n" ++
        "_note\n" ++
        ";\n" ++
        "never closed\n";
    if (s := summarizeCif(unterm_semi)) {
        * = s;
        expect_eq(true, false);
    }
    let lex = CifLexer.init(unterm_semi);
    * = lex.next(); // data_bad
    * = lex.next(); // _note
    if (t := lex.next()) {
        * = t;
        expect_eq(true, false);
    }
    expect_eq(lex.hasError(), true);

    // 3. Incomplete loop_ row (2 columns declared, 3 values present)
    const bad_loop =
        "data_badloop\n" ++
        "loop_\n" ++
        "_col_a\n" ++
        "_col_b\n" ++
        "1 2\n" ++
        "3\n";
    if (s := summarizeCif(bad_loop)) {
        * = s;
        expect_eq(true, false);
    }
    let loops = CifLoopIter.init(bad_loop);
    if (lp := loops.next()) {
        * = lp;
        expect_eq(true, false);
    }
    expect_eq(loops.hasError(), true);
}
