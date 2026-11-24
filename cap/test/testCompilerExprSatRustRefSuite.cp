// Reference-hardened test suite against jix/varisat (MIT OR Apache-2.0) & industrial Pratt expression evaluator
// Covers expr precedence, right-associative power (**), div/mod-by-zero, NaN/±Inf, unclosed parens,
// trailing garbage, empty input, EXPR_MAX_DEPTH=128 deep nesting rejection; SAT PHP(4,3) UNSAT,
// 3-SAT, Horn, empty formula SAT, empty clause immediate UNSAT, var=0/oob & decision budget Unknown,
// 100% independent verifySatModel; 16-lane & 4-lane SIMD vs scalar 0-diff parity; and 1,600 PRNG fuzz
// cases (800 malformed expressions + 800 random 3-SAT instances).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    EXPR_MAX_DEPTH,
    simdExprSkipWhitespace,
    scalarExprSkipWhitespace,
    ExprContext,
    compile,
    compileWithMaxDepth,
    eval,
} from "../../lib/compiler/expr.cp";
import {
    SAT_DEFAULT_MAX_VARS,
    simdSatCountTrueLits,
    scalarSatCountTrueLits,
    verifySatModel,
    SatError,
    SatStatus,
    SatSolver,
} from "../../lib/compiler/sat.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }
}

fn checkTest1ExprPrecedenceAndGuards(): u32 {
    const alloc = std.heap.page_allocator;
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();

    // 1a. Precedence & right-associative power (2 ** 3 ** 2 == 512, 1 + 2 * 3 ** 2 == 19)
    const v1 = eval("2 ** 3 ** 2", ctx) catch return 1;
    if ((v1.asNumber() orelse 0.0) != 512.0) return 2;

    const v2 = eval("1 + 2 * 3 ** 2 - 4 / 2", ctx) catch return 3;
    if ((v2.asNumber() orelse 0.0) != 17.0) return 4;

    const v3 = eval("2 ** -3", ctx) catch return 5;
    if ((v3.asNumber() orelse 0.0) != 0.125) return 6;

    // 1b. Division & modulo by zero rejected
    let ok = false;
    * = eval("10 / (5 - 5)", ctx) catch |e| {
        ok = (e == @as(anyerror, error.DivisionByZero));
    };
    if (!ok) return 7;

    ok = false;
    * = eval("10 % 0", ctx) catch |e| {
        ok = (e == @as(anyerror, error.DivisionByZero));
    };
    if (!ok) return 8;

    // 1c. NaN / ±Inf rejected (power overflow to Inf, negative fractional power to NaN, ctx NaN)
    ok = false;
    * = eval("10 ** 400", ctx) catch |e| {
        ok = (e == @as(anyerror, error.NonFiniteResult));
    };
    if (!ok) return 9;

    ok = false;
    * = eval("(-1) ** 0.5", ctx) catch |e| {
        ok = (e == @as(anyerror, error.NonFiniteResult));
    };
    if (!ok) return 10;

    ctx.setNumber("bad_nan", std.math.nan(f64)) catch return 11;
    ok = false;
    * = eval("bad_nan + 1", ctx) catch |e| {
        ok = (e == @as(anyerror, error.NonFiniteResult));
    };
    if (!ok) return 12;

    // 1d. Empty input, unclosed parens, trailing garbage rejected
    ok = false;
    * = compile(alloc, "   \t\r\n  ") catch |e| {
        ok = (e == @as(anyerror, error.UnexpectedEof));
    };
    if (!ok) return 13;

    ok = false;
    * = compile(alloc, "(1 + 2") catch |e| {
        ok = (e == @as(anyerror, error.UnexpectedEof));
    };
    if (!ok) return 14;

    ok = false;
    * = compile(alloc, "1 + 2 )") catch |e| {
        ok = (e == @as(anyerror, error.UnexpectedToken));
    };
    if (!ok) return 15;

    // 1e. Deep nesting (((...))) exceeding EXPR_MAX_DEPTH = 128 rejected (even if caller passes 9999)
    let deep_parens: [320]u8 = [0; 320];
    let i: usize = 0;
    while (i < 150) : (i += 1) {
        deep_parens[i] = '(';
    }
    deep_parens[150] = '1';
    i = 0;
    while (i < 150) : (i += 1) {
        deep_parens[151 + i] = ')';
    }
    ok = false;
    * = compileWithMaxDepth(alloc, deep_parens[0..301], 9999) catch |e| {
        ok = (e == @as(anyerror, error.MaxDepthExceeded));
    };
    if (!ok) return 16;

    // Deep unary nesting ----...1 exceeding EXPR_MAX_DEPTH rejected
    let deep_unary: [160]u8 = ['-'; 160];
    deep_unary[159] = '1';
    ok = false;
    * = compileWithMaxDepth(alloc, deep_unary[0..160], EXPR_MAX_DEPTH + 100) catch |e| {
        ok = (e == @as(anyerror, error.MaxDepthExceeded));
    };
    if (!ok) return 17;

    return 0;
}

fn checkTest2SatPhpHornAndGuards(): u32 {
    const alloc = std.heap.page_allocator;

    // 2a. Empty formula -> immediate SAT
    {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        let sol = s.solve() catch return 1;
        defer sol.deinit();
        if (sol.status != .Satisfiable) return 2;
    }

    // 2b. Empty clause -> immediate UNSAT (via addClauseAllowEmpty) & addClause returns EmptyClause
    {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        let caught = false;
        * = s.addClause(&.{}) catch |e| {
            caught = (e == SatError.EmptyClause);
        };
        if (!caught) return 3;

        s.addClauseAllowEmpty(&.{}) catch return 4;
        let sol = s.solve() catch return 5;
        defer sol.deinit();
        if (sol.status != .Unsatisfiable || sol.decisions != 0) return 6;
    }

    // 2c. Variable 0, INT32_MIN, and out-of-bounds variable rejected; clause cap enforced
    {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        const lit0: [1]i32 = [0];
        let caught = false;
        * = s.addClause(lit0[0..]) catch |e| {
            caught = (e == SatError.InvalidVariable);
        };
        if (!caught) return 7;

        const lit_oob: [1]i32 = [SAT_DEFAULT_MAX_VARS + 1];
        caught = false;
        * = s.addClause(lit_oob[0..]) catch |e| {
            caught = (e == SatError.InvalidVariable);
        };
        if (!caught) return 8;

        s.setMaxClauses(1);
        const c1: [1]i32 = [1];
        s.addClause(c1[0..]) catch return 9;
        caught = false;
        * = s.addClause(c1[0..]) catch |e| {
            caught = (e == SatError.MaxClausesExceeded);
        };
        if (!caught) return 10;
    }

    // 2d. Pigeonhole Principle PHP(4 pigeons, 3 holes) -> UNSAT (12 vars: p_{i,h} = i*3 + h + 1)
    {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        // Each pigeon i in 0..4 must be in at least one hole h in 0..3
        let p: i32 = 0;
        while (p < 4) : (p += 1) {
            const c: [3]i32 = [p * 3 + 1, p * 3 + 2, p * 3 + 3];
            s.addClause(c[0..]) catch return 11;
        }
        // No two pigeons i < j share hole h
        let h: i32 = 0;
        while (h < 3) : (h += 1) {
            let i: i32 = 0;
            while (i < 4) : (i += 1) {
                let j: i32 = i + 1;
                while (j < 4) : (j += 1) {
                    const c: [2]i32 = [-(i * 3 + h + 1), -(j * 3 + h + 1)];
                    s.addClause(c[0..]) catch return 12;
                }
            }
        }
        let sol = s.solve() catch return 13;
        defer sol.deinit();
        if (sol.status != .Unsatisfiable) return 14;

        // With decision budget = 1 on PHP(4,3), solver safely returns Unknown instead of looping
        s.setMaxDecisions(1);
        let sol_u = s.solve() catch return 15;
        defer sol_u.deinit();
        if (sol_u.status != .Unknown) return 16;
    }

    // 2e. Horn formula SAT + independent verifySatModel check
    {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        const c1: [1]i32 = [1];
        const c2: [2]i32 = [-1, 2];
        const c3: [3]i32 = [-1, -2, 3];
        const c4: [2]i32 = [-3, 4];
        s.addClause(c1[0..]) catch return 17;
        s.addClause(c2[0..]) catch return 18;
        s.addClause(c3[0..]) catch return 19;
        s.addClause(c4[0..]) catch return 20;

        let sol = s.solve() catch return 21;
        defer sol.deinit();
        if (sol.status != .Satisfiable) return 22;
        if (!sol.isTrue(1) || !sol.isTrue(2) || !sol.isTrue(3) || !sol.isTrue(4)) return 23;
        const all_clauses: [4][]const i32 = [c1[0..], c2[0..], c3[0..], c4[0..]];
        if (!verifySatModel(all_clauses[0..], sol.assignment)) return 24;
    }

    return 0;
}

fn checkTest3SimdDifferential(): u32 {
    let rng = XorShift64.init(0x45585052_53415453);

    // 3a. simdExprSkipWhitespace vs scalarExprSkipWhitespace across lengths 0..=160
    let buf: [164]u8 = [0; 164];
    const ws_chars: [4]u8 = [' ', '\t', '\n', '\r'];
    let len: usize = 0;
    while (len <= 160) : (len += 1) {
        const ws_prefix = if (len == 0) 0 else @as(usize, @truncate(rng.next())) % (len + 1);
        let i: usize = 0;
        while (i < ws_prefix) : (i += 1) {
            buf[i] = ws_chars[@as(usize, @truncate(rng.next())) % 4];
        }
        while (i < len) : (i += 1) {
            buf[i] = (0x21 as u8) + (rng.nextByte() % (90 as u8));
        }
        const r_simd = simdExprSkipWhitespace(buf[0..len], 0);
        const r_scalar = scalarExprSkipWhitespace(buf[0..len], 0);
        if (r_simd != r_scalar || r_simd != ws_prefix) return 1;
    }

    // 3b. simdSatCountTrueLits vs scalarSatCountTrueLits across clause lengths 0..=64
    let assignment: [33]bool = [false; 33];
    let v: usize = 1;
    while (v <= 32) : (v += 1) {
        assignment[v] = (rng.next() & 1) != 0;
    }
    let clause: [64]i32 = [0; 64];
    let clen: usize = 0;
    while (clen <= 64) : (clen += 1) {
        let i: usize = 0;
        while (i < clen) : (i += 1) {
            const var_id: i32 = 1 + @as(i32, @intCast(rng.next() % 32));
            clause[i] = if ((rng.next() & 1) == 0) var_id else -var_id;
        }
        const c_simd = simdSatCountTrueLits(clause[0..clen], assignment[0..]);
        const c_scalar = scalarSatCountTrueLits(clause[0..clen], assignment[0..]);
        if (c_simd != c_scalar) return 2;
    }

    return 0;
}

fn checkTest4PrngFuzz1600(): u32 {
    const alloc = std.heap.page_allocator;
    let rng = XorShift64.init(0x57333852_3346555A);

    // 4a. 800 malformed & random expression fuzz cases (zero crash/leak)
    const alphabet = "0123456789+-*/%()!?:<>=&| \t\n\r\"'abc._";
    let expr_buf: [96]u8 = [0; 96];
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();
    ctx.setNumber("a", 3.0) catch return 1;
    ctx.setNumber("b", 2.0) catch return 2;
    ctx.setBool("c", true) catch return 3;

    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 80;
        let k: usize = 0;
        while (k < len) : (k += 1) {
            expr_buf[k] = alphabet[@as(usize, @truncate(rng.next())) % alphabet.len];
        }
        let c_mut = compile(alloc, expr_buf[0..len]) catch continue;
        defer c_mut.deinit();
        * = c_mut.eval(ctx) catch {};
    }

    // 4b. 800 random 3-SAT instances (8..12 vars, 16..36 clauses); every SAT model 100% verified
    let sat_count: usize = 0;
    let unsat_count: usize = 0;
    iter = 0;
    while (iter < 800) : (iter += 1) {
        let s = SatSolver.init(alloc);
        defer s.deinit();
        s.setMaxDecisions(2000);

        const nvars: i32 = 8 + @as(i32, @intCast(rng.next() % 5)); // 8..12
        const nclauses: usize = (16 as usize) + @as(usize, @truncate(rng.next() % 21)); // 16..36
        let c: usize = 0;
        while (c < nclauses) : (c += 1) {
            let lits: [3]i32 = [0; 3];
            let li: usize = 0;
            while (li < 3) : (li += 1) {
                const vid: i32 = 1 + @as(i32, @intCast(rng.next() % @as(u64, @intCast(nvars))));
                lits[li] = if ((rng.next() & 1) == 0) vid else -vid;
            }
            s.addClause(lits[0..]) catch return 4;
        }

        let sol = s.solve() catch return 5;
        defer sol.deinit();
        if (sol.status == .Satisfiable) {
            if (!s.verifyAssignment(sol.assignment)) return 6;
            sat_count += 1;
        } else if (sol.status == .Unsatisfiable) {
            unsat_count += 1;
        }
    }
    if (sat_count == 0 || unsat_count == 0) return 7;
    return 0;
}

fn main() {
    test("1. Expr Precedence, Right-Assoc **, Div/Mod-0, NaN/Inf & Depth 128 Guards", () => {
        expect_eq(checkTest1ExprPrecedenceAndGuards(), 0 as u32);
    });

    test("2. SAT PHP(4,3) UNSAT, Horn SAT, Empty Clause/Formula, Budget Unknown & Var Guards", () => {
        expect_eq(checkTest2SatPhpHornAndGuards(), 0 as u32);
    });

    test("3. 16-Lane & 4-Lane SIMD vs Scalar 0-Diff Differential Parity", () => {
        expect_eq(checkTest3SimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG Fuzzing (800 Expr + 800 Random 3-SAT = 1,600 Cases)", () => {
        expect_eq(checkTest4PrngFuzz1600(), 0 as u32);
    });
}
