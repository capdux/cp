// Source: cp/lib/sat/solver.cp (DPLL SAT Solver, DIMACS CNF Parser & High-Level ConstraintModel)
// Pure CP, zero-C-dependency Boolean Satisfiability (SAT) solver with DPLL, unit propagation,
// pure literal elimination, MOM branching heuristic, DIMACS CNF parser, and named ConstraintModel.

const std = @import("std");

/// Error set for SAT solver, DIMACS parser, and constraint modeling operations.
pub err SatError {
    InvalidVariable,
    EmptyClause,
    InvalidDimacs,
    MaxDecisionsExceeded,
    OutOfMemory,
}

/// Satisfiability verdict returned by `SatSolver.solve()`.
pub enum SatStatus {
    Satisfiable,
    Unsatisfiable,
}

/// Solution returned by `SatSolver.solve()`.
/// Call `deinit()` to release the owned assignment slice.
pub struct SatSolution {
    alloc: std.mem.Allocator;
    status: SatStatus;
    assignment: []bool;
    decisions: u64;
    propagations: u64;

    /// Releases the assignment array allocated during solving.
    fn deinit(*_) {
        if (_.assignment.len > 0) {
            _.alloc.free(_.assignment);
            _.assignment = [];
        }
    }

    /// Returns true if 1-based variable `var_id` is assigned `true` in this solution.
    fn isTrue(var_id: i32): bool {
        if (var_id <= 0) return false;
        const idx = usize(var_id);
        if (idx >= _.assignment.len) return false;
        return _.assignment[idx];
    }
}

/// Solution returned by `ConstraintModel.solveNamed()`.
/// Owns its copied variable names and boolean values; call `deinit()` to free.
pub struct NamedSolution {
    alloc: std.mem.Allocator;
    names: [][]u8;
    values: []bool;
    decisions: u64;
    propagations: u64;

    /// Releases all owned variable names and value arrays.
    fn deinit(*_) {
        for (i of 0.._.names.len) {
            _.alloc.free(_.names[i]);
        }
        if (_.names.len > 0) {
            _.alloc.free(_.names);
            _.names = [];
        }
        if (_.values.len > 0) {
            _.alloc.free(_.values);
            _.values = [];
        }
    }

    /// Returns true if the named variable was assigned `true`, or `false` if unassigned/unknown.
    fn isSelected(name: string): bool {
        for (i of 0.._.names.len) {
            if (std.mem.eql(u8, _.names[i], name)) {
                return _.values[i];
            }
        }
        return false;
    }
}

fn absVar(lit: i32): usize {
    return if (lit > 0) usize(lit) else usize(-lit);
}

fn evalLit(vals: []const i8, lit: i32): i8 {
    const v = absVar(lit);
    const val = vals[v];
    if (val == 0) return 0;
    if ((lit > 0 && val == 1) || (lit < 0 && val == -1)) return 1;
    return -1;
}

fn isWhitespace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn parseSignedInt(tok: string): i32? {
    if (tok.len == 0) return null;
    let idx: usize = 0;
    let neg = false;
    if (tok[0] == '-') {
        neg = true;
        idx = 1;
    } else if (tok[0] == '+') {
        idx = 1;
    }
    if (idx >= tok.len) return null;

    let acc: i64 = 0;
    while (idx < tok.len) : (idx += 1) {
        const ch = tok[idx];
        if (ch < '0' || ch > '9') return null;
        acc = acc * 10 + i64(ch - '0');
        if (acc > 2147483647) return null;
    }
    const res: i32 = if (neg) i32(-acc) else i32(acc);
    return res;
}

fn parseNonNegI32(tok: string): i32? {
    const v = parseSignedInt(tok) orelse return null;
    if (v < 0) return null;
    return v;
}

struct DpllState {
    clauses: []const []i32;
    num_vars: usize;
    vals: []i8;
    trail: []i32;
    trail_len: usize;
    pos_cnt: []u32;
    neg_cnt: []u32;
    decisions: u64;
    propagations: u64;
    max_decisions: u64;

    fn assignVar(*_, var_id: i32, val: i8) {
        const idx = usize(var_id);
        if (_.vals[idx] == 0) {
            _.vals[idx] = val;
            _.trail[_.trail_len] = var_id;
            _.trail_len += 1;
        }
    }

    fn rollback(*_, target_len: usize) {
        while (_.trail_len > target_len) {
            _.trail_len -= 1;
            const v = _.trail[_.trail_len];
            _.vals[usize(v)] = 0;
        }
    }

    fn propagate(*_): bool {
        let changed = true;
        while (changed) {
            changed = false;
            for (c_idx of 0.._.clauses.len) {
                const clause = _.clauses[c_idx];
                let satisfied = false;
                let unassigned_count: usize = 0;
                let last_unassigned: i32 = 0;

                for (l_idx of 0..clause.len) {
                    const lit = clause[l_idx];
                    const ev = evalLit(_.vals, lit);
                    if (ev == 1) {
                        satisfied = true;
                        break;
                    }
                    if (ev == 0) {
                        unassigned_count += 1;
                        last_unassigned = lit;
                    }
                }

                if (satisfied) continue;
                if (unassigned_count == 0) {
                    return false;
                }
                if (unassigned_count == 1) {
                    const v = i32(absVar(last_unassigned));
                    const assign_val: i8 = if (last_unassigned > 0) 1 else -1;
                    _.assignVar(v, assign_val);
                    _.propagations += 1;
                    changed = true;
                }
            }
        }
        return true;
    }

    fn eliminatePureLiterals(*_): bool {
        let assigned_any = false;
        @memset(_.pos_cnt, 0);
        @memset(_.neg_cnt, 0);

        for (c_idx of 0.._.clauses.len) {
            const clause = _.clauses[c_idx];
            let satisfied = false;
            for (l_idx of 0..clause.len) {
                if (evalLit(_.vals, clause[l_idx]) == 1) {
                    satisfied = true;
                    break;
                }
            }
            if (satisfied) continue;

            for (l_idx of 0..clause.len) {
                const lit = clause[l_idx];
                if (evalLit(_.vals, lit) == 0) {
                    const v = absVar(lit);
                    if (lit > 0) {
                        _.pos_cnt[v] += 1;
                    } else {
                        _.neg_cnt[v] += 1;
                    }
                }
            }
        }

        for (v of 1..(_.num_vars + 1)) {
            if (_.vals[v] != 0) continue;
            const p = _.pos_cnt[v];
            const n = _.neg_cnt[v];
            if (p > 0 && n == 0) {
                _.assignVar(i32(v), 1);
                _.propagations += 1;
                assigned_any = true;
            } else if (n > 0 && p == 0) {
                _.assignVar(i32(v), -1);
                _.propagations += 1;
                assigned_any = true;
            }
        }
        return assigned_any;
    }

    fn search(*_): bool ! SatError {
        while (true) {
            if (!_.propagate()) return false;
            if (!_.eliminatePureLiterals()) break;
        }

        // Find minimum-size unsatisfied clause for MOM heuristic.
        let min_len: usize = 0;
        for (c_idx of 0.._.clauses.len) {
            const clause = _.clauses[c_idx];
            let satisfied = false;
            let open_cnt: usize = 0;
            for (l_idx of 0..clause.len) {
                const ev = evalLit(_.vals, clause[l_idx]);
                if (ev == 1) {
                    satisfied = true;
                    break;
                }
                if (ev == 0) {
                    open_cnt += 1;
                }
            }
            if (!satisfied) {
                if (open_cnt == 0) return false;
                if (min_len == 0 || open_cnt < min_len) {
                    min_len = open_cnt;
                }
            }
        }

        // All clauses satisfied.
        if (min_len == 0) return true;

        // Score unassigned variables in minimum-length unsatisfied clauses (MOM).
        @memset(_.pos_cnt, 0);
        @memset(_.neg_cnt, 0);
        for (c_idx of 0.._.clauses.len) {
            const clause = _.clauses[c_idx];
            let satisfied = false;
            let open_cnt: usize = 0;
            for (l_idx of 0..clause.len) {
                const ev = evalLit(_.vals, clause[l_idx]);
                if (ev == 1) {
                    satisfied = true;
                    break;
                }
                if (ev == 0) {
                    open_cnt += 1;
                }
            }
            if (satisfied || open_cnt != min_len) continue;

            for (l_idx of 0..clause.len) {
                const lit = clause[l_idx];
                if (evalLit(_.vals, lit) == 0) {
                    const v = absVar(lit);
                    if (lit > 0) {
                        _.pos_cnt[v] += 1;
                    } else {
                        _.neg_cnt[v] += 1;
                    }
                }
            }
        }

        let best_var: usize = 0;
        let best_score: u64 = 0;
        let first_val: i8 = 1;

        for (v of 1..(_.num_vars + 1)) {
            if (_.vals[v] != 0) continue;
            const p = u64(_.pos_cnt[v]);
            const n = u64(_.neg_cnt[v]);
            const score = (p + n) * 16 + p * n + 1;
            if (best_var == 0 || score > best_score) {
                best_var = v;
                best_score = score;
                first_val = if (p >= n) 1 else -1;
            }
        }

        if (best_var == 0) return true;

        _.decisions += 1;
        if (_.decisions > _.max_decisions) {
            return SatError.MaxDecisionsExceeded;
        }

        const saved = _.trail_len;
        _.assignVar(i32(best_var), first_val);
        const ok1 = _.search() catch |e| return e;
        if (ok1) return true;
        _.rollback(saved);

        _.assignVar(i32(best_var), -first_val);
        const ok2 = _.search() catch |e| return e;
        if (ok2) return true;
        _.rollback(saved);

        return false;
    }
}

/// DPLL-based Boolean Satisfiability (SAT) solver using 1-based signed integer literals.
/// Positive `k` represents variable `k` = true; negative `-k` represents variable `k` = false.
pub struct SatSolver {
    alloc: std.mem.Allocator;
    num_vars: i32 = 0;
    clauses: [][]i32 = [];
    clauses_len: usize = 0;
    max_decisions: u64 = 10000000;

    /// Initializes an empty SAT solver.
    static fn init(alloc: std.mem.Allocator): SatSolver {
        return SatSolver{
            alloc: alloc,
            num_vars: 0,
            clauses: [],
            clauses_len: 0,
            max_decisions: 10000000,
        };
    }

    /// Releases all clauses owned by this solver.
    fn deinit(*_) {
        for (i of 0.._.clauses_len) {
            _.alloc.free(_.clauses[i]);
        }
        if (_.clauses.len > 0) {
            _.alloc.free(_.clauses);
            _.clauses = [];
        }
        _.clauses_len = 0;
        _.num_vars = 0;
    }

    /// Configures the maximum number of DPLL branching decisions before aborting.
    fn setMaxDecisions(*_, limit: u64) {
        _.max_decisions = limit;
    }

    /// Allocates and returns a fresh 1-based variable ID.
    fn newVar(*_): i32 {
        _.num_vars += 1;
        return _.num_vars;
    }

    /// Ensures at least `n` variables are registered.
    fn ensureVars(*_, n: i32): void ! SatError {
        if (n < 0) return SatError.InvalidVariable;
        if (n > _.num_vars) {
            _.num_vars = n;
        }
    }

    /// Adds a disjunctive clause `(lits[0] | lits[1] | ...)`.
    /// Rejects empty clauses (`EmptyClause`) and literal `0` (`InvalidVariable`).
    fn addClause(*_, lits: []const i32): void ! SatError {
        if (lits.len == 0) return SatError.EmptyClause;

        let max_v = _.num_vars;
        for (i of 0..lits.len) {
            const lit = lits[i];
            if (lit == 0) return SatError.InvalidVariable;
            const v = if (lit > 0) lit else -lit;
            if (v > max_v) {
                max_v = v;
            }
        }

        const copy = _.alloc.alloc(i32, lits.len) catch return SatError.OutOfMemory;
        @memcpy(copy, lits);
        errdefer _.alloc.free(copy);

        if (_.clauses_len == _.clauses.len) {
            const new_cap: usize = if (_.clauses.len == 0) 8 else _.clauses.len * 2;
            const new_buf = _.alloc.alloc([]i32, new_cap) catch return SatError.OutOfMemory;
            if (_.clauses_len > 0) {
                @memcpy(new_buf[0.._.clauses_len], _.clauses[0.._.clauses_len]);
                _.alloc.free(_.clauses);
            }
            _.clauses = new_buf;
        }

        _.clauses[_.clauses_len] = copy;
        _.clauses_len += 1;
        _.num_vars = max_v;
    }

    /// Parses a standard DIMACS CNF string (`c` comments, `p cnf <vars> <clauses>`, `0`-terminated clauses).
    static fn parseDimacs(alloc: std.mem.Allocator, cnf_text: string): SatSolver ! SatError {
        let solver = SatSolver.init(alloc);
        errdefer solver.deinit();

        let header_seen = false;
        let expected_clauses: usize = 0;
        let parsed_clauses: usize = 0;

        let cur_lits: []i32 = [];
        let cur_len: usize = 0;
        defer if (cur_lits.len > 0) alloc.free(cur_lits);

        let pos: usize = 0;
        let at_line_start = true;

        while (pos < cnf_text.len) {
            const ch = cnf_text[pos];
            if (ch == '\n') {
                pos += 1;
                at_line_start = true;
                continue;
            }
            if (isWhitespace(ch)) {
                pos += 1;
                continue;
            }

            if (at_line_start && ch == 'c') {
                while (pos < cnf_text.len && cnf_text[pos] != '\n') : (pos += 1) {}
                continue;
            }

            if (at_line_start && ch == '%') {
                break;
            }

            if (at_line_start && ch == 'p') {
                if (header_seen) return SatError.InvalidDimacs;
                let line_end = pos;
                while (line_end < cnf_text.len && cnf_text[line_end] != '\n') : (line_end += 1) {}
                const line = cnf_text[pos..line_end];

                // Tokenize "p cnf <vars> <clauses>"
                let t_idx: usize = 0;
                let tok_count: usize = 0;
                let vars_val: i32 = 0;
                let clauses_val: i32 = 0;

                while (t_idx < line.len) {
                    while (t_idx < line.len && isWhitespace(line[t_idx])) : (t_idx += 1) {}
                    if (t_idx >= line.len) break;
                    const t_start = t_idx;
                    while (t_idx < line.len && !isWhitespace(line[t_idx])) : (t_idx += 1) {}
                    const tok = line[t_start..t_idx];

                    if (tok_count == 0) {
                        if (!std.mem.eql(u8, tok, "p")) return SatError.InvalidDimacs;
                    } else if (tok_count == 1) {
                        if (!std.mem.eql(u8, tok, "cnf")) return SatError.InvalidDimacs;
                    } else if (tok_count == 2) {
                        vars_val = parseNonNegI32(tok) orelse return SatError.InvalidDimacs;
                    } else if (tok_count == 3) {
                        clauses_val = parseNonNegI32(tok) orelse return SatError.InvalidDimacs;
                    } else {
                        return SatError.InvalidDimacs;
                    }
                    tok_count += 1;
                }

                if (tok_count != 4) return SatError.InvalidDimacs;
                solver.ensureVars(vars_val) catch |e| return e;
                expected_clauses = usize(clauses_val);
                header_seen = true;
                pos = line_end;
                at_line_start = false;
                continue;
            }

            if (!header_seen) {
                return SatError.InvalidDimacs;
            }

            at_line_start = false;
            const tok_start = pos;
            while (pos < cnf_text.len && !isWhitespace(cnf_text[pos])) : (pos += 1) {}
            const tok = cnf_text[tok_start..pos];

            const lit = parseSignedInt(tok) orelse return SatError.InvalidDimacs;
            if (lit == 0) {
                if (cur_len == 0) return SatError.EmptyClause;
                solver.addClause(cur_lits[0..cur_len]) catch |e| return e;
                parsed_clauses += 1;
                cur_len = 0;
            } else {
                if (cur_len == cur_lits.len) {
                    const new_cap: usize = if (cur_lits.len == 0) 8 else cur_lits.len * 2;
                    const new_buf = alloc.alloc(i32, new_cap) catch return SatError.OutOfMemory;
                    if (cur_len > 0) {
                        @memcpy(new_buf[0..cur_len], cur_lits[0..cur_len]);
                        alloc.free(cur_lits);
                    }
                    cur_lits = new_buf;
                }
                cur_lits[cur_len] = lit;
                cur_len += 1;
            }
        }

        if (!header_seen) return SatError.InvalidDimacs;
        if (cur_len > 0) return SatError.InvalidDimacs;
        if (parsed_clauses != expected_clauses) return SatError.InvalidDimacs;

        return solver;
    }

    /// Solves the CNF instance via DPLL with unit propagation, pure literal elimination, and MOM heuristic.
    /// Verifies 100% of original clauses before returning `Satisfiable`.
    fn solve(): SatSolution ! SatError {
        const nvars = usize(_.num_vars);
        const assign_len = nvars + 1;

        let assignment = _.alloc.alloc(bool, assign_len) catch return SatError.OutOfMemory;
        errdefer _.alloc.free(assignment);
        @memset(assignment, false);

        if (_.clauses_len == 0) {
            return SatSolution{
                alloc: _.alloc,
                status: .Satisfiable,
                assignment: assignment,
                decisions: 0,
                propagations: 0,
            };
        }

        const vals = _.alloc.alloc(i8, assign_len) catch return SatError.OutOfMemory;
        defer _.alloc.free(vals);
        @memset(vals, 0);

        const trail = _.alloc.alloc(i32, assign_len) catch return SatError.OutOfMemory;
        defer _.alloc.free(trail);

        const pos_cnt = _.alloc.alloc(u32, assign_len) catch return SatError.OutOfMemory;
        defer _.alloc.free(pos_cnt);

        const neg_cnt = _.alloc.alloc(u32, assign_len) catch return SatError.OutOfMemory;
        defer _.alloc.free(neg_cnt);

        let state = DpllState{
            clauses: _.clauses[0.._.clauses_len],
            num_vars: nvars,
            vals: vals,
            trail: trail,
            trail_len: 0,
            pos_cnt: pos_cnt,
            neg_cnt: neg_cnt,
            decisions: 0,
            propagations: 0,
            max_decisions: _.max_decisions,
        };

        const sat = state.search() catch |e| return e;
        if (!sat) {
            return SatSolution{
                alloc: _.alloc,
                status: .Unsatisfiable,
                assignment: assignment,
                decisions: state.decisions,
                propagations: state.propagations,
            };
        }

        for (v of 1..assign_len) {
            assignment[v] = vals[v] == 1;
        }

        // Verify all original clauses evaluate to true under the final assignment.
        for (c_idx of 0.._.clauses_len) {
            const clause = _.clauses[c_idx];
            let clause_sat = false;
            for (l_idx of 0..clause.len) {
                const lit = clause[l_idx];
                const v = absVar(lit);
                const val = assignment[v];
                if ((lit > 0 && val) || (lit < 0 && !val)) {
                    clause_sat = true;
                    break;
                }
            }
            if (!clause_sat) {
                return SatError.InvalidDimacs;
            }
        }

        return SatSolution{
            alloc: _.alloc,
            status: .Satisfiable,
            assignment: assignment,
            decisions: state.decisions,
            propagations: state.propagations,
        };
    }
}

/// High-level declarative Boolean constraint builder with named variables.
/// Ideal for package dependency resolution, scheduling, and configuration validation.
pub struct ConstraintModel {
    alloc: std.mem.Allocator;
    solver: SatSolver;
    names: [][]u8 = [];
    names_len: usize = 0;

    /// Initializes an empty `ConstraintModel`.
    static fn init(alloc: std.mem.Allocator): ConstraintModel {
        return ConstraintModel{
            alloc: alloc,
            solver: SatSolver.init(alloc),
            names: [],
            names_len: 0,
        };
    }

    /// Releases the underlying SAT solver and all registered variable names.
    fn deinit(*_) {
        _.solver.deinit();
        for (i of 0.._.names_len) {
            _.alloc.free(_.names[i]);
        }
        if (_.names.len > 0) {
            _.alloc.free(_.names);
            _.names = [];
        }
        _.names_len = 0;
    }

    /// Declares (or retrieves) the 1-based SAT variable ID for `name`.
    fn declare(*_, name: string): i32 ! SatError {
        for (i of 0.._.names_len) {
            if (std.mem.eql(u8, _.names[i], name)) {
                return i32(i) + 1;
            }
        }

        const name_copy = _.alloc.alloc(u8, name.len) catch return SatError.OutOfMemory;
        @memcpy(name_copy, name);
        errdefer _.alloc.free(name_copy);

        if (_.names_len == _.names.len) {
            const new_cap: usize = if (_.names.len == 0) 8 else _.names.len * 2;
            const new_buf = _.alloc.alloc([]u8, new_cap) catch return SatError.OutOfMemory;
            if (_.names_len > 0) {
                @memcpy(new_buf[0.._.names_len], _.names[0.._.names_len]);
                _.alloc.free(_.names);
            }
            _.names = new_buf;
        }

        const var_id = _.solver.newVar();
        _.names[_.names_len] = name_copy;
        _.names_len += 1;
        return var_id;
    }

    /// Constrains `name` to be `true`.
    fn requireTrue(*_, name: string): void ! SatError {
        const v = _.declare(name) catch |e| return e;
        const lits: [1]i32 = [v];
        _.solver.addClause(&lits) catch |e| return e;
    }

    /// Constrains `name` to be `false`.
    fn requireFalse(*_, name: string): void ! SatError {
        const v = _.declare(name) catch |e| return e;
        const lits: [1]i32 = [-v];
        _.solver.addClause(&lits) catch |e| return e;
    }

    /// Constrains `a => b` (`!a | b`).
    fn implies(*_, a: string, b: string): void ! SatError {
        const va = _.declare(a) catch |e| return e;
        const vb = _.declare(b) catch |e| return e;
        const lits: [2]i32 = [-va, vb];
        _.solver.addClause(&lits) catch |e| return e;
    }

    /// Constrains `a => (choices[0] | choices[1] | ...)`.
    fn impliesAny(*_, a: string, choices: []const string): void ! SatError {
        const va = _.declare(a) catch |e| return e;
        let buf = _.alloc.alloc(i32, choices.len + 1) catch return SatError.OutOfMemory;
        defer _.alloc.free(buf);
        buf[0] = -va;
        for (i of 0..choices.len) {
            buf[i + 1] = _.declare(choices[i]) catch |e| return e;
        }
        _.solver.addClause(buf) catch |e| return e;
    }

    /// Constrains `!(a & b)` (`!a | !b`).
    fn conflicts(*_, a: string, b: string): void ! SatError {
        const va = _.declare(a) catch |e| return e;
        const vb = _.declare(b) catch |e| return e;
        const lits: [2]i32 = [-va, -vb];
        _.solver.addClause(&lits) catch |e| return e;
    }

    /// Constrains that all variables in `names` must be `true`.
    fn allOf(*_, names: []const string): void ! SatError {
        for (i of 0..names.len) {
            _.requireTrue(names[i]) catch |e| return e;
        }
    }

    /// Constrains that at least one variable in `names` must be `true`.
    fn anyOf(*_, names: []const string): void ! SatError {
        if (names.len == 0) return SatError.EmptyClause;
        let buf = _.alloc.alloc(i32, names.len) catch return SatError.OutOfMemory;
        defer _.alloc.free(buf);
        for (i of 0..names.len) {
            buf[i] = _.declare(names[i]) catch |e| return e;
        }
        _.solver.addClause(buf) catch |e| return e;
    }

    /// Constrains that at most one variable in `names` may be `true`.
    fn atMostOne(*_, names: []const string): void ! SatError {
        for (i of 0..names.len) {
            const vi = _.declare(names[i]) catch |e| return e;
            for (j of (i + 1)..names.len) {
                const vj = _.declare(names[j]) catch |e| return e;
                const lits: [2]i32 = [-vi, -vj];
                _.solver.addClause(&lits) catch |e| return e;
            }
        }
    }

    /// Constrains that exactly one variable in `names` must be `true`.
    fn exactlyOne(*_, names: []const string): void ! SatError {
        _.anyOf(names) catch |e| return e;
        _.atMostOne(names) catch |e| return e;
    }

    /// Constrains `(conds[0] & ... & conds[k-1]) => target` (`!conds[0] | ... | !conds[k-1] | target`).
    fn requireIfAll(*_, conds: []const string, target: string): void ! SatError {
        let buf = _.alloc.alloc(i32, conds.len + 1) catch return SatError.OutOfMemory;
        defer _.alloc.free(buf);
        for (i of 0..conds.len) {
            const vc = _.declare(conds[i]) catch |e| return e;
            buf[i] = -vc;
        }
        buf[conds.len] = _.declare(target) catch |e| return e;
        _.solver.addClause(buf) catch |e| return e;
    }

    /// Solves the constraint model and returns a `NamedSolution` if satisfiable, or `null` if unsatisfiable.
    fn solveNamed(alloc: std.mem.Allocator): NamedSolution? ! SatError {
        let sol = _.solver.solve() catch |e| return e;
        defer sol.deinit();

        if (sol.status == .Unsatisfiable) {
            return null;
        }

        let out_names = alloc.alloc([]u8, _.names_len) catch return SatError.OutOfMemory;
        let copied_count: usize = 0;
        errdefer {
            for (k of 0..copied_count) {
                alloc.free(out_names[k]);
            }
            alloc.free(out_names);
        }

        let out_vals = alloc.alloc(bool, _.names_len) catch return SatError.OutOfMemory;
        errdefer alloc.free(out_vals);

        for (i of 0.._.names_len) {
            const src = _.names[i];
            const dup = alloc.alloc(u8, src.len) catch return SatError.OutOfMemory;
            @memcpy(dup, src);
            out_names[i] = dup;
            copied_count += 1;
            out_vals[i] = sol.isTrue(i32(i) + 1);
        }

        return NamedSolution{
            alloc: alloc,
            names: out_names,
            values: out_vals,
            decisions: sol.decisions,
            propagations: sol.propagations,
        };
    }
}
