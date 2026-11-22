// Source: cp/cap/test/test_sat_production_deep.cp
// Deep Production Verification Suite for cap:sat & cap.sat
// Covers:
//   1. Classic 3-SAT satisfiable verification & Pigeonhole Principle PHP(4,3) UNSAT proof
//   2. Standard DIMACS CNF parsing & malformed input / EmptyClause / literal-0 rejection
//   3. ConstraintModel package dependency conflict & version mutual-exclusion resolution
//   4. 4-Queens (N=4) declarative modeling & solution verification under std.testing.allocator (zero leaks)
//   5. `import cap from "cap"; cap.sat` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SatError,
    SatStatus,
    SatSolver,
    ConstraintModel,
} from "cap:compiler";

test("cap:sat - 3-SAT satisfiable verification and Pigeonhole PHP(4,3) UNSAT") {
    const alloc = std.testing.allocator;

    // 1) Satisfiable 3-SAT instance
    let sat = SatSolver.init(alloc);
    defer sat.deinit();

    const x1 = sat.newVar();
    const x2 = sat.newVar();
    const x3 = sat.newVar();
    const x4 = sat.newVar();

    sat.addClause(&[x1, x2, -x3]) catch unreachable;
    sat.addClause(&[-x1, x3, x4]) catch unreachable;
    sat.addClause(&[-x2, -x3, x4]) catch unreachable;
    sat.addClause(&[x1, -x4]) catch unreachable;
    sat.addClause(&[x2, x3]) catch unreachable;

    let sol = sat.solve() catch unreachable;
    defer sol.deinit();

    expect_eq(sol.status == .Satisfiable, true);
    const v1 = sol.isTrue(x1);
    const v2 = sol.isTrue(x2);
    const v3 = sol.isTrue(x3);
    const v4 = sol.isTrue(x4);

    // Independently verify every clause holds
    expect_eq(v1 || v2 || !v3, true);
    expect_eq(!v1 || v3 || v4, true);
    expect_eq(!v2 || !v3 || v4, true);
    expect_eq(v1 || !v4, true);
    expect_eq(v2 || v3, true);

    // 2) Pigeonhole Principle PHP(4, 3): 4 pigeons into 3 holes -> strictly UNSAT
    let php = SatSolver.init(alloc);
    defer php.deinit();
    php.ensureVars(12) catch unreachable;

    // Each pigeon i in 0..4 must be in at least one hole j in 0..3
    for (i of 0..4) {
        const base = i32(i * 3) + 1;
        php.addClause(&[base, base + 1, base + 2]) catch unreachable;
    }

    // No hole j in 0..3 can hold two pigeons i1 < i2
    for (j of 0..3) {
        for (i1 of 0..4) {
            for (i2 of (i1 + 1)..4) {
                const v_a = i32(i1 * 3 + j) + 1;
                const v_b = i32(i2 * 3 + j) + 1;
                php.addClause(&[-v_a, -v_b]) catch unreachable;
            }
        }
    }

    let php_sol = php.solve() catch unreachable;
    defer php_sol.deinit();
    expect_eq(php_sol.status == .Unsatisfiable, true);
}

test("cap:sat - DIMACS CNF parsing and invalid input rejection") {
    const alloc = std.testing.allocator;

    const cnf =
        "c Sample DIMACS CNF\n" ++
        "c 4 variables, 4 clauses\n" ++
        "p cnf 4 4\n" ++
        "1 2 0\n" ++
        "-1 3 0\n" ++
        "-2 -3 4\n" ++
        "0\n" ++
        "-4 1 0\n";

    let solver = SatSolver.parseDimacs(alloc, cnf) catch unreachable;
    defer solver.deinit();

    let sol = solver.solve() catch unreachable;
    defer sol.deinit();
    expect_eq(sol.status == .Satisfiable, true);

    const a1 = sol.isTrue(1);
    const a2 = sol.isTrue(2);
    const a3 = sol.isTrue(3);
    const a4 = sol.isTrue(4);
    expect_eq(a1 || a2, true);
    expect_eq(!a1 || a3, true);
    expect_eq(!a2 || !a3 || a4, true);
    expect_eq(!a4 || a1, true);

    // Reject missing header
    let bad_header = false;
    * = SatSolver.parseDimacs(alloc, "1 2 0\n") catch e => {
        if (e == SatError.InvalidDimacs) bad_header = true;
    };
    expect_eq(bad_header, true);

    // Reject empty clause in DIMACS (standalone 0)
    let empty_dimacs = false;
    * = SatSolver.parseDimacs(alloc, "p cnf 2 1\n0\n") catch e => {
        if (e == SatError.EmptyClause) empty_dimacs = true;
    };
    expect_eq(empty_dimacs, true);

    // Reject literal 0 and empty slice in addClause
    let manual = SatSolver.init(alloc);
    defer manual.deinit();

    let zero_lit = false;
    manual.addClause(&[1, 0]) catch e => {
        if (e == SatError.InvalidVariable) zero_lit = true;
    };
    expect_eq(zero_lit, true);

    let empty_clause = false;
    const empty_slice: [0]i32 = [];
    manual.addClause(&empty_slice) catch e => {
        if (e == SatError.EmptyClause) empty_clause = true;
    };
    expect_eq(empty_clause, true);
}

test("cap:sat - ConstraintModel package dependency conflict resolution") {
    const alloc = std.testing.allocator;
    let model = ConstraintModel.init(alloc);
    defer model.deinit();

    // app -> libA_v1 | libA_v2, exactlyOne(libA_v1, libA_v2), plugin -> libA_v2
    model.impliesAny("app", &["libA_v1", "libA_v2"]) catch unreachable;
    model.exactlyOne(&["libA_v1", "libA_v2"]) catch unreachable;
    model.implies("plugin", "libA_v2") catch unreachable;
    model.requireIfAll(&["app", "plugin"], "strict_mode") catch unreachable;
    model.conflicts("libA_v1", "strict_mode") catch unreachable;
    model.allOf(&["app", "plugin"]) catch unreachable;

    let maybe_sol = model.solveNamed(alloc) catch unreachable;
    expect_eq(maybe_sol != null, true);

    let sol = maybe_sol.?;
    defer sol.deinit();

    expect_eq(sol.isSelected("app"), true);
    expect_eq(sol.isSelected("plugin"), true);
    expect_eq(sol.isSelected("libA_v2"), true);
    expect_eq(sol.isSelected("libA_v1"), false);
    expect_eq(sol.isSelected("strict_mode"), true);

    // Contradictory model must return null
    let bad_model = ConstraintModel.init(alloc);
    defer bad_model.deinit();
    bad_model.allOf(&["pkg_a", "pkg_b"]) catch unreachable;
    bad_model.conflicts("pkg_a", "pkg_b") catch unreachable;

    let no_sol = bad_model.solveNamed(alloc) catch unreachable;
    expect_eq(no_sol == null, true);
}

test("cap:sat - 4-Queens problem with std.testing.allocator zero leaks") {
    const alloc = std.testing.allocator;
    let model = ConstraintModel.init(alloc);
    defer model.deinit();

    // 4x4 board variable names
    const cells: [4][4]string = [
        ["q00", "q01", "q02", "q03"],
        ["q10", "q11", "q12", "q13"],
        ["q20", "q21", "q22", "q23"],
        ["q30", "q31", "q32", "q33"],
    ];

    // Exactly one queen per row
    for (r of 0..4) {
        model.exactlyOne(&cells[r]) catch unreachable;
    }

    // Exactly one queen per column
    for (c of 0..4) {
        const col: [4]string = [cells[0][c], cells[1][c], cells[2][c], cells[3][c]];
        model.exactlyOne(&col) catch unreachable;
    }

    // Diagonal conflicts
    for (r1 of 0..4) {
        for (c1 of 0..4) {
            for (r2 of (r1 + 1)..4) {
                for (c2 of 0..4) {
                    const dr = i32(r2) - i32(r1);
                    const dc = if (c2 >= c1) i32(c2) - i32(c1) else i32(c1) - i32(c2);
                    if (dr == dc) {
                        model.conflicts(cells[r1][c1], cells[r2][c2]) catch unreachable;
                    }
                }
            }
        }
    }

    let maybe_sol = model.solveNamed(alloc) catch unreachable;
    expect_eq(maybe_sol != null, true);
    let sol = maybe_sol.?;
    defer sol.deinit();

    let queen_cols: [4]i32 = [0, 0, 0, 0];
    let total_queens: usize = 0;
    for (r of 0..4) {
        let row_cnt: usize = 0;
        for (c of 0..4) {
            if (sol.isSelected(cells[r][c])) {
                row_cnt += 1;
                total_queens += 1;
                queen_cols[r] = i32(c);
            }
        }
        expect_eq(row_cnt, 1);
    }
    expect_eq(total_queens, 4);

    // Verify no two queens share column or diagonal
    for (r1 of 0..4) {
        for (r2 of (r1 + 1)..4) {
            const c1 = queen_cols[r1];
            const c2 = queen_cols[r2];
            expect_eq(c1 != c2, true);
            const dr = i32(r2) - i32(r1);
            const dc = if (c2 >= c1) c2 - c1 else c1 - c2;
            expect_eq(dr != dc, true);
        }
    }
}

test("cap:sat - cap.sat dynamic mounting") {
    const alloc = std.testing.allocator;

    let solver = cap.compiler.SatSolver.init(alloc);
    defer solver.deinit();

    const a = solver.newVar();
    const b = solver.newVar();
    solver.addClause(&[a, b]) catch unreachable;
    solver.addClause(&[-a, b]) catch unreachable;

    let sol = solver.solve() catch unreachable;
    defer sol.deinit();

    expect_eq(sol.status == cap.compiler.SatStatus.Satisfiable, true);
    expect_eq(sol.isTrue(b), true);

    let model = cap.compiler.ConstraintModel.init(alloc);
    defer model.deinit();
    model.requireTrue("enable_tls") catch unreachable;
    model.implies("enable_tls", "cert_ready") catch unreachable;

    let nsol = (model.solveNamed(alloc) catch unreachable).?;
    defer nsol.deinit();
    expect_eq(nsol.isSelected("cert_ready"), true);
}
