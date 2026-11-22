// Source: cp/cap/test/test_hungarian_production_deep.cp (Production deep tests for cap:hungarian)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AssignmentResult,
    AssignmentResultF64,
    AssignmentResultI64,
    HungarianWorkspace,
    HungarianWorkspaceF64,
    HungarianWorkspaceI64,
    solveMinF64,
    solveMaxF64,
    solveMinI64,
    solveMaxI64,
} from "cap:graph";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn bruteMinI64_4x4(cost: []const i64): i64 {
    let best: i64 = 900000000000000;
    let c0: usize = 0;
    while (c0 < 4) {
        let c1: usize = 0;
        while (c1 < 4) {
            if (c1 != c0) {
                let c2: usize = 0;
                while (c2 < 4) {
                    if (c2 != c0 && c2 != c1) {
                        const c3: usize = (6 as usize) - c0 - c1 - c2;
                        const s: i64 = cost[0 * 4 + c0] + cost[1 * 4 + c1] + cost[2 * 4 + c2] + cost[3 * 4 + c3];
                        if (s < best) {
                            best = s;
                        }
                    }
                    c2 += 1;
                }
            }
            c1 += 1;
        }
        c0 += 1;
    }
    return best;
}

fn bruteMaxI64_4x4(cost: []const i64): i64 {
    let best: i64 = -900000000000000;
    let c0: usize = 0;
    while (c0 < 4) {
        let c1: usize = 0;
        while (c1 < 4) {
            if (c1 != c0) {
                let c2: usize = 0;
                while (c2 < 4) {
                    if (c2 != c0 && c2 != c1) {
                        const c3: usize = (6 as usize) - c0 - c1 - c2;
                        const s: i64 = cost[0 * 4 + c0] + cost[1 * 4 + c1] + cost[2 * 4 + c2] + cost[3 * 4 + c3];
                        if (s > best) {
                            best = s;
                        }
                    }
                    c2 += 1;
                }
            }
            c1 += 1;
        }
        c0 += 1;
    }
    return best;
}

test("1. Classic 3x3 & 4x4 min assignment matches brute-force permutation optimum", () => {
    // 3x3 textbook matrix: optimal = 4+0+2=6? No: (0,1)=1, (1,0)=2, (2,2)=2 -> sum = 5
    const c3: [9]i64 = [
        4, 1, 3,
        2, 0, 5,
        3, 2, 2,
    ];
    let u3: [4]i64 = std.mem.zeroes([4]i64);
    let v3: [4]i64 = std.mem.zeroes([4]i64);
    let mv3: [4]i64 = std.mem.zeroes([4]i64);
    let p3: [4]usize = std.mem.zeroes([4]usize);
    let w3: [4]usize = std.mem.zeroes([4]usize);
    let us3: [4]u8 = std.mem.zeroes([4]u8);
    let ws3: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: u3[0..], v: v3[0..], minv: mv3[0..], p: p3[0..], way: w3[0..], used: us3[0..],
    };
    let a3_buf: [3]i32 = std.mem.zeroes([3]i32);
    let a3: []i32 = a3_buf[0..];

    const r3: AssignmentResultI64 = solveMinI64(c3[0..], 3, 3, *a3, *ws3) ?? unreachable;
    expect_eq(r3.matchedCount, 3 as usize);
    expect_eq(r3.totalCost, 5 as i64);
    expect_eq(a3[0], 1 as i32);
    expect_eq(a3[1], 0 as i32);
    expect_eq(a3[2], 2 as i32);

    // 4x4 matrix compared against 4! = 24 brute-force permutations
    const c4: [16]i64 = [
        82, 83, 69, 92,
        77, 37, 49, 92,
        11, 69,  5, 86,
         8,  9, 98, 23,
    ];
    let u4: [5]i64 = std.mem.zeroes([5]i64);
    let v4: [5]i64 = std.mem.zeroes([5]i64);
    let mv4: [5]i64 = std.mem.zeroes([5]i64);
    let p4: [5]usize = std.mem.zeroes([5]usize);
    let w4: [5]usize = std.mem.zeroes([5]usize);
    let us4: [5]u8 = std.mem.zeroes([5]u8);
    let ws4: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: u4[0..], v: v4[0..], minv: mv4[0..], p: p4[0..], way: w4[0..], used: us4[0..],
    };
    let a4_buf: [4]i32 = std.mem.zeroes([4]i32);
    let a4: []i32 = a4_buf[0..];

    const r4: AssignmentResultI64 = solveMinI64(c4[0..], 4, 4, *a4, *ws4) ?? unreachable;
    const expected4: i64 = bruteMinI64_4x4(c4[0..]);
    expect_eq(r4.matchedCount, 4 as usize);
    expect_eq(r4.totalCost, expected4);
    // Verify sum of selected cells matches totalCost and columns are distinct
    let sum4: i64 = 0;
    let col_seen: [4]u8 = std.mem.zeroes([4]u8);
    let row: usize = 0;
    while (row < 4) {
        const col: usize = a4[row] as usize;
        expect_eq(col_seen[col], 0 as u8);
        col_seen[col] = 1;
        sum4 += c4[row * 4 + col];
        row += 1;
    }
    expect_eq(sum4, expected4);
});

test("2. Rectangular matrices: rows < cols (3x5) and rows > cols (5x3)", () => {
    // 3x5 matrix: optimal picks (0,2)=1, (1,4)=2, (2,0)=1 -> total = 4
    const wide: [15]i64 = [
        9, 8, 1, 7, 6,
        5, 6, 4, 8, 2,
        1, 9, 5, 6, 7,
    ];
    let u: [6]i64 = std.mem.zeroes([6]i64);
    let v: [6]i64 = std.mem.zeroes([6]i64);
    let mv: [6]i64 = std.mem.zeroes([6]i64);
    let p: [6]usize = std.mem.zeroes([6]usize);
    let w: [6]usize = std.mem.zeroes([6]usize);
    let us: [6]u8 = std.mem.zeroes([6]u8);
    let ws: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: u[0..], v: v[0..], minv: mv[0..], p: p[0..], way: w[0..], used: us[0..],
    };

    let a_wide_buf: [3]i32 = std.mem.zeroes([3]i32);
    let a_wide: []i32 = a_wide_buf[0..];
    const r_wide: AssignmentResultI64 = solveMinI64(wide[0..], 3, 5, *a_wide, *ws) ?? unreachable;
    expect_eq(r_wide.matchedCount, 3 as usize);
    expect_eq(r_wide.totalCost, 4 as i64);
    expect_eq(a_wide[0], 2 as i32);
    expect_eq(a_wide[1], 4 as i32);
    expect_eq(a_wide[2], 0 as i32);

    // 5x3 transpose of the above matrix: rows > cols, 3 rows matched, 2 rows == -1
    const tall: [15]i64 = [
        9, 5, 1,
        8, 6, 9,
        1, 4, 5,
        7, 8, 6,
        6, 2, 7,
    ];
    let a_tall_buf: [5]i32 = std.mem.zeroes([5]i32);
    let a_tall: []i32 = a_tall_buf[0..];
    const r_tall: AssignmentResultI64 = solveMinI64(tall[0..], 5, 3, *a_tall, *ws) ?? unreachable;
    expect_eq(r_tall.matchedCount, 3 as usize);
    expect_eq(r_tall.totalCost, 4 as i64);
    expect_eq(a_tall[0], 2 as i32);
    expect_eq(a_tall[1], -1 as i32);
    expect_eq(a_tall[2], 0 as i32);
    expect_eq(a_tall[3], -1 as i32);
    expect_eq(a_tall[4], 1 as i32);
});

test("3. Negative weights & maximum-weight assignment (solveMaxI64 & solveMaxF64)", () => {
    const w4: [16]i64 = [
        -10,  25, -3,  12,
         18, -20, 30,  -5,
         -7,  14,  8,  22,
         19,  -1, -9,  15,
    ];
    let u: [5]i64 = std.mem.zeroes([5]i64);
    let v: [5]i64 = std.mem.zeroes([5]i64);
    let mv: [5]i64 = std.mem.zeroes([5]i64);
    let p: [5]usize = std.mem.zeroes([5]usize);
    let way: [5]usize = std.mem.zeroes([5]usize);
    let used: [5]u8 = std.mem.zeroes([5]u8);
    let ws: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: u[0..], v: v[0..], minv: mv[0..], p: p[0..], way: way[0..], used: used[0..],
    };
    let a_buf: [4]i32 = std.mem.zeroes([4]i32);
    let a: []i32 = a_buf[0..];

    const res_max: AssignmentResultI64 = solveMaxI64(w4[0..], 4, 4, *a, *ws) ?? unreachable;
    const expected_max: i64 = bruteMaxI64_4x4(w4[0..]);
    expect_eq(res_max.matchedCount, 4 as usize);
    expect_eq(res_max.totalCost, expected_max);

    const res_min: AssignmentResultI64 = solveMinI64(w4[0..], 4, 4, *a, *ws) ?? unreachable;
    const expected_min: i64 = bruteMinI64_4x4(w4[0..]);
    expect_eq(res_min.totalCost, expected_min);

    // Floating-point solveMaxF64 on identical weights
    let wf: [16]f64 = std.mem.zeroes([16]f64);
    let idx: usize = 0;
    while (idx < 16) {
        wf[idx] = w4[idx] as f64;
        idx += 1;
    }
    let uf: [5]f64 = std.mem.zeroes([5]f64);
    let vf: [5]f64 = std.mem.zeroes([5]f64);
    let mvf: [5]f64 = std.mem.zeroes([5]f64);
    let wsf: HungarianWorkspace = HungarianWorkspace {
        u: uf[0..], v: vf[0..], minv: mvf[0..], p: p[0..], way: way[0..], used: used[0..],
    };
    const res_max_f: AssignmentResult = solveMaxF64(wf[0..], 4, 4, *a, *wsf) ?? unreachable;
    expect_eq(absF64(res_max_f.totalCost - (expected_max as f64)) < 1.0e-12, true);
});

test("4. f64 precision & tie-breaking on degenerate constant and micro-separated costs", () => {
    // All-equal 4x4 matrix (all permutations tied at cost 10.0)
    const tied: [16]f64 = [
        2.5, 2.5, 2.5, 2.5,
        2.5, 2.5, 2.5, 2.5,
        2.5, 2.5, 2.5, 2.5,
        2.5, 2.5, 2.5, 2.5,
    ];
    let u: [5]f64 = std.mem.zeroes([5]f64);
    let v: [5]f64 = std.mem.zeroes([5]f64);
    let mv: [5]f64 = std.mem.zeroes([5]f64);
    let p: [5]usize = std.mem.zeroes([5]usize);
    let way: [5]usize = std.mem.zeroes([5]usize);
    let used: [5]u8 = std.mem.zeroes([5]u8);
    let ws: HungarianWorkspaceF64 = HungarianWorkspaceF64 {
        u: u[0..], v: v[0..], minv: mv[0..], p: p[0..], way: way[0..], used: used[0..],
    };
    let a_buf: [4]i32 = std.mem.zeroes([4]i32);
    let a: []i32 = a_buf[0..];

    const rt: AssignmentResultF64 = solveMinF64(tied[0..], 4, 4, *a, *ws) ?? unreachable;
    expect_eq(rt.matchedCount, 4 as usize);
    expect_eq(absF64(rt.totalCost - 10.0) < 1.0e-12, true);

    // Fine 1e-9 separation: unique optimum on anti-diagonal (0,2), (1,1), (2,0)
    const eps: f64 = 1.0e-9;
    const fine: [9]f64 = [
        1.0,       1.0,       1.0 - eps,
        1.0,       1.0 - eps, 1.0,
        1.0 - eps, 1.0,       1.0,
    ];
    const rf: AssignmentResultF64 = solveMinF64(fine[0..], 3, 3, *a, *ws) ?? unreachable;
    expect_eq(rf.matchedCount, 3 as usize);
    expect_eq(a[0], 2 as i32);
    expect_eq(a[1], 1 as i32);
    expect_eq(a[2], 0 as i32);
    expect_eq(absF64(rf.totalCost - (3.0 - 3.0 * eps)) < 1.0e-13, true);
});

test("5. 20x20 deterministic random matrix dual optimality (u[i] + v[j] <= cost[i][j] & complementary slackness)", () => {
    const N: usize = 20;
    let cost_i: [400]i64 = std.mem.zeroes([400]i64);
    let cost_f: [400]f64 = std.mem.zeroes([400]f64);

    // Deterministic SplitMix64 generator
    let state: u64 = 0x123456789ABCDEF0;
    let k: usize = 0;
    while (k < 400) {
        state = state +% 0x9E3779B97F4A7C15;
        let z: u64 = state;
        z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
        z = z ^ (z >> 31);
        const val: i64 = ((z % 2001) as i64) - 1000;
        cost_i[k] = val;
        cost_f[k] = (val as f64) * 0.125;
        k += 1;
    }

    // Exact i64 dual optimality verification
    let ui: [21]i64 = std.mem.zeroes([21]i64);
    let vi: [21]i64 = std.mem.zeroes([21]i64);
    let mvi: [21]i64 = std.mem.zeroes([21]i64);
    let pi: [21]usize = std.mem.zeroes([21]usize);
    let wayi: [21]usize = std.mem.zeroes([21]usize);
    let usedi: [21]u8 = std.mem.zeroes([21]u8);
    let wsi: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: ui[0..], v: vi[0..], minv: mvi[0..], p: pi[0..], way: wayi[0..], used: usedi[0..],
    };
    let ai_buf: [20]i32 = std.mem.zeroes([20]i32);
    let ai: []i32 = ai_buf[0..];

    const ri: AssignmentResultI64 = solveMinI64(cost_i[0..], N, N, *ai, *wsi) ?? unreachable;
    expect_eq(ri.matchedCount, N);

    // Verify dual feasibility: u[i+1] + v[j+1] <= cost[i][j] for all (i, j)
    let r: usize = 0;
    while (r < N) {
        let c: usize = 0;
        while (c < N) {
            expect_eq(wsi.u[r + 1] + wsi.v[c + 1] <= cost_i[r * N + c], true);
            c += 1;
        }
        r += 1;
    }

    // Verify complementary slackness & strong duality:
    // u[r+1] + v[col+1] == cost[r][col] and sum(u) + sum(v) == totalCost
    let dual_sum_i: i64 = 0;
    let seen_cols: [20]u8 = std.mem.zeroes([20]u8);
    let r2: usize = 0;
    while (r2 < N) {
        const col: usize = ai[r2] as usize;
        expect_eq(col < N, true);
        expect_eq(seen_cols[col], 0 as u8);
        seen_cols[col] = 1;
        expect_eq(wsi.u[r2 + 1] + wsi.v[col + 1], cost_i[r2 * N + col]);
        dual_sum_i += wsi.u[r2 + 1] + wsi.v[r2 + 1];
        r2 += 1;
    }
    expect_eq(dual_sum_i, ri.totalCost);

    // f64 dual optimality verification on scaled matrix
    let uf: [21]f64 = std.mem.zeroes([21]f64);
    let vf: [21]f64 = std.mem.zeroes([21]f64);
    let mvf: [21]f64 = std.mem.zeroes([21]f64);
    let wsf: HungarianWorkspaceF64 = HungarianWorkspaceF64 {
        u: uf[0..], v: vf[0..], minv: mvf[0..], p: pi[0..], way: wayi[0..], used: usedi[0..],
    };
    let af_buf: [20]i32 = std.mem.zeroes([20]i32);
    let af: []i32 = af_buf[0..];

    const rf: AssignmentResultF64 = solveMinF64(cost_f[0..], N, N, *af, *wsf) ?? unreachable;
    expect_eq(rf.matchedCount, N);
    expect_eq(absF64(rf.totalCost - ((ri.totalCost as f64) * 0.125)) < 1.0e-10, true);

    let rf_r: usize = 0;
    while (rf_r < N) {
        let rf_c: usize = 0;
        while (rf_c < N) {
            expect_eq(wsf.u[rf_r + 1] + wsf.v[rf_c + 1] <= cost_f[rf_r * N + rf_c] + 1.0e-10, true);
            rf_c += 1;
        }
        const mcol: usize = af[rf_r] as usize;
        expect_eq(absF64((wsf.u[rf_r + 1] + wsf.v[mcol + 1]) - cost_f[rf_r * N + mcol]) < 1.0e-10, true);
        rf_r += 1;
    }
});

test("6. Input validation: empty matrix, short buffers, NaN/Inf & i64 overflow guards", () => {
    const ok_f: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const ok_i: [4]i64 = [1, 2, 3, 4];

    let uf: [3]f64 = std.mem.zeroes([3]f64);
    let vf: [3]f64 = std.mem.zeroes([3]f64);
    let mvf: [3]f64 = std.mem.zeroes([3]f64);
    let ui: [3]i64 = std.mem.zeroes([3]i64);
    let vi: [3]i64 = std.mem.zeroes([3]i64);
    let mvi: [3]i64 = std.mem.zeroes([3]i64);
    let p: [3]usize = std.mem.zeroes([3]usize);
    let way: [3]usize = std.mem.zeroes([3]usize);
    let used: [3]u8 = std.mem.zeroes([3]u8);

    let wsf: HungarianWorkspaceF64 = HungarianWorkspaceF64 {
        u: uf[0..], v: vf[0..], minv: mvf[0..], p: p[0..], way: way[0..], used: used[0..],
    };
    let wsi: HungarianWorkspaceI64 = HungarianWorkspaceI64 {
        u: ui[0..], v: vi[0..], minv: mvi[0..], p: p[0..], way: way[0..], used: used[0..],
    };

    let a_buf: [2]i32 = std.mem.zeroes([2]i32);
    let a: []i32 = a_buf[0..];
    let short_a: []i32 = a_buf[0..1];

    // Empty dimensions
    expect_eq(solveMinF64(ok_f[0..], 0, 2, *a, *wsf) == null, true);
    expect_eq(solveMinF64(ok_f[0..], 2, 0, *a, *wsf) == null, true);
    expect_eq(solveMinI64(ok_i[0..], 0, 2, *a, *wsi) == null, true);
    expect_eq(solveMinI64(ok_i[0..], 2, 0, *a, *wsi) == null, true);

    // Insufficient cost slice or rowToCol
    expect_eq(solveMinF64(ok_f[0..3], 2, 2, *a, *wsf) == null, true);
    expect_eq(solveMinF64(ok_f[0..], 2, 2, *short_a, *wsf) == null, true);
    expect_eq(solveMinI64(ok_i[0..3], 2, 2, *a, *wsi) == null, true);
    expect_eq(solveMinI64(ok_i[0..], 2, 2, *short_a, *wsi) == null, true);

    // Insufficient workspace slice (needs 2 + 1 = 3, given 2)
    let short_wsf: HungarianWorkspaceF64 = HungarianWorkspaceF64 {
        u: uf[0..2], v: vf[0..], minv: mvf[0..], p: p[0..], way: way[0..], used: used[0..],
    };
    expect_eq(solveMinF64(ok_f[0..], 2, 2, *a, *short_wsf) == null, true);

    // NaN & Inf rejection
    const zero_f: f64 = 0.0;
    const nan_val: f64 = zero_f / zero_f;
    const inf_val: f64 = 1.0 / zero_f;
    const nan_mat: [4]f64 = [1.0, nan_val, 3.0, 4.0];
    const inf_mat: [4]f64 = [1.0, 2.0, inf_val, 4.0];
    expect_eq(solveMinF64(nan_mat[0..], 2, 2, *a, *wsf) == null, true);
    expect_eq(solveMaxF64(inf_mat[0..], 2, 2, *a, *wsf) == null, true);

    // i64 extreme overflow guard
    const huge_mat: [4]i64 = [1, 2000000000000000, 3, 4];
    expect_eq(solveMinI64(huge_mat[0..], 2, 2, *a, *wsi) == null, true);
});
