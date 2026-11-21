// Source: cp/cap/test/test_simplex_production_deep.cp (Production deep tests for cap:simplex)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ConstraintKind,
    SimplexStatus,
    SimplexResult,
    SimplexWorkspace,
    simplexRequiredTableauLen,
    solveGeneral,
    solveMaxLe,
    solveMinLe,
} from "cap:simplex";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn dotF64(a: []const f64, b: []const f64, n: usize): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        s += a[i] * b[i];
        i += 1;
    }
    return s;
}

test("1. Classic production scheduling LP maximization matches analytical primal & dual", () => {
    // Hillier & Lieberman Wyndor Glass problem:
    // max 3*x1 + 5*x2
    // s.t.   x1          <= 4
    //              2*x2  <= 12
    //      3*x1 +  2*x2  <= 18
    //      x1, x2 >= 0
    // Exact optimum: x = (2, 6), Z = 36, dual y = (0, 1.5, 1.0)
    const c: [2]f64 = [3.0, 5.0];
    const a: [6]f64 = [
        1.0, 0.0,
        0.0, 2.0,
        3.0, 2.0,
    ];
    const b: [3]f64 = [4.0, 12.0, 18.0];

    expect_eq(simplexRequiredTableauLen(2, 3), 36 as usize);

    let tab: [36]f64 = std.mem.zeroes([36]f64);
    let basis: [3]usize = std.mem.zeroes([3]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let y_buf: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_buf[0..];
    let y: []f64 = y_buf[0..];

    const res: SimplexResult = solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 100, *x, *y, *ws);
    expect_eq(res.status == SimplexStatus.Optimal, true);
    expect_eq(absF64(res.objective - 36.0) < 1.0e-9, true);
    expect_eq(absF64(x[0] - 2.0) < 1.0e-9, true);
    expect_eq(absF64(x[1] - 6.0) < 1.0e-9, true);

    // Shadow prices & strong duality c^T x == b^T y
    expect_eq(absF64(y[0] - 0.0) < 1.0e-9, true);
    expect_eq(absF64(y[1] - 1.5) < 1.0e-9, true);
    expect_eq(absF64(y[2] - 1.0) < 1.0e-9, true);
    const dual_obj: f64 = dotF64(b[0..], y, 3);
    expect_eq(absF64(dual_obj - res.objective) < 1.0e-9, true);
});

test("2. solveMinLe with negative RHS triggers Phase I and reaches exact minimum", () => {
    // min 2*x1 + 3*x2
    // s.t. -x1 - x2 <= -4   (i.e. x1 + x2 >= 4)
    //     -2*x1 - x2 <= -6  (i.e. 2*x1 + x2 >= 6)
    //      x1, x2 >= 0
    // Vertices: (4, 0) -> 8? Wait: 2*4 + 0 = 8 >= 6, 4 + 0 = 4 >= 4 -> Z = 8!
    // Check: (4, 0) satisfies x1+x2=4>=4, 2x1+x2=8>=6 -> 2*4 + 3*0 = 8!
    // Wait: (2, 2) gives 4+6=10, (0, 6) gives 18. Minimum is (4, 0) with Z = 8!
    const c: [2]f64 = [2.0, 3.0];
    const a: [4]f64 = [
        -1.0, -1.0,
        -2.0, -1.0,
    ];
    const b: [2]f64 = [-4.0, -6.0];

    let tab: [21]f64 = std.mem.zeroes([21]f64); // (2+1)*(2+4+1) = 21
    let basis: [2]usize = std.mem.zeroes([2]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let y_buf: [2]f64 = std.mem.zeroes([2]f64);
    let x: []f64 = x_buf[0..];
    let y: []f64 = y_buf[0..];

    const res: SimplexResult = solveMinLe(c[0..], a[0..], b[0..], 2, 2, 100, *x, *y, *ws);
    expect_eq(res.status == SimplexStatus.Optimal, true);
    expect_eq(absF64(res.objective - 8.0) < 1.0e-9, true);
    expect_eq(absF64(x[0] - 4.0) < 1.0e-9, true);
    expect_eq(absF64(x[1] - 0.0) < 1.0e-9, true);

    // Strong duality: b^T y == c^T x == 8.0
    const dual_obj: f64 = dotF64(b[0..], y, 2);
    expect_eq(absF64(dual_obj - res.objective) < 1.0e-9, true);
});

test("3. Two-Phase solveGeneral with mixed Eq, Ge & Le constraints and strong duality", () => {
    // Classic Two-Phase LP:
    // min 4*x1 + x2
    // s.t. 3*x1 + x2  = 3
    //      4*x1 + 3*x2 >= 6
    //        x1 + 2*x2 <= 4
    //      x1, x2 >= 0
    // Intersection of 3x1+x2=3 and x1+2x2=4: 5x2=9 -> x2=1.8, x1=0.4 -> Z = 1.6 + 1.8 = 3.4
    // Check 4*0.4 + 3*1.8 = 1.6 + 5.4 = 7.0 >= 6 (strictly feasible).
    const c: [2]f64 = [4.0, 1.0];
    const a: [6]f64 = [
        3.0, 1.0,
        4.0, 3.0,
        1.0, 2.0,
    ];
    const b: [3]f64 = [3.0, 6.0, 4.0];
    const kinds: [3]ConstraintKind = [
        ConstraintKind.Eq,
        ConstraintKind.Ge,
        ConstraintKind.Le,
    ];

    let tab: [36]f64 = std.mem.zeroes([36]f64);
    let basis: [3]usize = std.mem.zeroes([3]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let y_buf: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_buf[0..];
    let y: []f64 = y_buf[0..];

    const res: SimplexResult = solveGeneral(
        c[0..], a[0..], b[0..], kinds[0..], 2, 3, false, 100, *x, *y, *ws,
    );
    expect_eq(res.status == SimplexStatus.Optimal, true);
    expect_eq(absF64(x[0] - 0.4) < 1.0e-9, true);
    expect_eq(absF64(x[1] - 1.8) < 1.0e-9, true);
    expect_eq(absF64(res.objective - 3.4) < 1.0e-9, true);

    // Verify all 3 mixed constraints hold
    expect_eq(absF64((3.0 * x[0] + x[1]) - 3.0) < 1.0e-9, true);
    expect_eq((4.0 * x[0] + 3.0 * x[1]) >= 6.0 - 1.0e-9, true);
    expect_eq((x[0] + 2.0 * x[1]) <= 4.0 + 1.0e-9, true);

    // Strong duality: b^T y == c^T x == 3.4
    const dual_obj: f64 = dotF64(b[0..], y, 3);
    expect_eq(absF64(dual_obj - res.objective) < 1.0e-9, true);
});

test("4. Infeasible LP detected accurately by Phase I", () => {
    // Contradictory constraints: x1 + x2 <= 2 AND x1 + x2 >= 5
    const c: [2]f64 = [1.0, 1.0];
    const a: [4]f64 = [
        1.0, 1.0,
        1.0, 1.0,
    ];
    const b: [2]f64 = [2.0, 5.0];
    const kinds: [2]ConstraintKind = [ConstraintKind.Le, ConstraintKind.Ge];

    let tab: [21]f64 = std.mem.zeroes([21]f64);
    let basis: [2]usize = std.mem.zeroes([2]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let empty_y: [0]f64 = [];
    let x: []f64 = x_buf[0..];
    let y: []f64 = empty_y[0..];

    const res: SimplexResult = solveGeneral(
        c[0..], a[0..], b[0..], kinds[0..], 2, 2, true, 100, *x, *y, *ws,
    );
    expect_eq(res.status == SimplexStatus.Infeasible, true);
});

test("5. Unbounded LP detected accurately in Phase II", () => {
    // max x1 + 2*x2
    // s.t. -x1 + x2 <= 1
    //      -2*x1 + x2 <= 2
    //      x1, x2 >= 0  (unbounded along ray x1 = t, x2 = t -> +inf)
    const c: [2]f64 = [1.0, 2.0];
    const a: [4]f64 = [
        -1.0, 1.0,
        -2.0, 1.0,
    ];
    const b: [2]f64 = [1.0, 2.0];

    let tab: [21]f64 = std.mem.zeroes([21]f64);
    let basis: [2]usize = std.mem.zeroes([2]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let empty_y: [0]f64 = [];
    let x: []f64 = x_buf[0..];
    let y: []f64 = empty_y[0..];

    const res: SimplexResult = solveMaxLe(c[0..], a[0..], b[0..], 2, 2, 100, *x, *y, *ws);
    expect_eq(res.status == SimplexStatus.Unbounded, true);
});

test("6. Beale (1955) degenerate cycling benchmark converges via Bland's rule & satisfies strong duality", () => {
    // Beale's famous cycling example:
    // max 0.75*x1 - 20*x2 + 0.5*x3 - 6*x4
    // s.t. 0.25*x1 -  8*x2 -     x3 + 9*x4 <= 0
    //       0.5*x1 - 12*x2 - 0.5*x3 + 3*x4 <= 0
    //                            x3        <= 1
    //      x >= 0
    // Exact optimum: x = (1, 0, 1, 0), Z = 1.25
    const c: [4]f64 = [0.75, -20.0, 0.5, -6.0];
    const a: [12]f64 = [
        0.25, -8.0, -1.0, 9.0,
        0.5, -12.0, -0.5, 3.0,
        0.0,   0.0,  1.0, 0.0,
    ];
    const b: [3]f64 = [0.0, 0.0, 1.0];

    // Worst-case tableau: (3+1) * (4 + 2*3 + 1) = 44
    let tab: [44]f64 = std.mem.zeroes([44]f64);
    let basis: [3]usize = std.mem.zeroes([3]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [4]f64 = std.mem.zeroes([4]f64);
    let y_buf: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_buf[0..];
    let y: []f64 = y_buf[0..];

    const res: SimplexResult = solveMaxLe(c[0..], a[0..], b[0..], 4, 3, 200, *x, *y, *ws);
    expect_eq(res.status == SimplexStatus.Optimal, true);
    expect_eq(absF64(res.objective - 1.25) < 1.0e-9, true);
    expect_eq(absF64(x[0] - 1.0) < 1.0e-9, true);
    expect_eq(absF64(x[1] - 0.0) < 1.0e-9, true);
    expect_eq(absF64(x[2] - 1.0) < 1.0e-9, true);
    expect_eq(absF64(x[3] - 0.0) < 1.0e-9, true);

    // Strong duality check: b^T y == c^T x == 1.25
    const dual_obj: f64 = dotF64(b[0..], y, 3);
    expect_eq(absF64(dual_obj - 1.25) < 1.0e-9, true);
});

test("7. Invalid inputs, short buffers, NaN/Inf & MaxIterations protection", () => {
    const c: [2]f64 = [3.0, 5.0];
    const a: [6]f64 = [1.0, 0.0, 0.0, 2.0, 3.0, 2.0];
    const b: [3]f64 = [4.0, 12.0, 18.0];

    let tab: [36]f64 = std.mem.zeroes([36]f64);
    let basis: [3]usize = std.mem.zeroes([3]usize);
    let ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..], basis: basis[0..] };

    let x_buf: [2]f64 = std.mem.zeroes([2]f64);
    let y_buf: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_buf[0..];
    let y: []f64 = y_buf[0..];

    // Zero dimensions
    expect_eq(solveMaxLe(c[0..], a[0..], b[0..], 0, 3, 50, *x, *y, *ws).status == SimplexStatus.InvalidInput, true);
    expect_eq(solveMaxLe(c[0..], a[0..], b[0..], 2, 0, 50, *x, *y, *ws).status == SimplexStatus.InvalidInput, true);

    // Short tableau / basis / x_out / non-empty short dual_out
    let short_ws: SimplexWorkspace = SimplexWorkspace { tableau: tab[0..10], basis: basis[0..] };
    expect_eq(solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 50, *x, *y, *short_ws).status == SimplexStatus.InvalidInput, true);

    let short_x: []f64 = x_buf[0..1];
    let short_y: []f64 = y_buf[0..2];
    expect_eq(solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 50, *short_x, *y, *ws).status == SimplexStatus.InvalidInput, true);
    expect_eq(solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 50, *x, *short_y, *ws).status == SimplexStatus.InvalidInput, true);

    // NaN / Inf guard
    const zero: f64 = 0.0;
    const nan_c: [2]f64 = [zero / zero, 5.0];
    const inf_b: [3]f64 = [4.0, 1.0 / zero, 18.0];
    expect_eq(solveMaxLe(nan_c[0..], a[0..], b[0..], 2, 3, 50, *x, *y, *ws).status == SimplexStatus.InvalidInput, true);
    expect_eq(solveMaxLe(c[0..], a[0..], inf_b[0..], 2, 3, 50, *x, *y, *ws).status == SimplexStatus.InvalidInput, true);

    // MaxIterations == 0 stops before optimal pivot
    const r_cap: SimplexResult = solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 0, *x, *y, *ws);
    expect_eq(r_cap.status == SimplexStatus.MaxIterations, true);
});
