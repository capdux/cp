// Source: Absorbed from /data/dev/bun/src/js_parser/ast/repl_transforms.cp | Synced: 2026-11-22T19:16:57Z
// ============================================================================
// lib/js/replTransform.cp — Zero-Alloc Interactive REPL AST Transform Planner
// ============================================================================
// Implements Bun's `js_parser/ast/repl_transforms.cp` transformation planning
// in 100% pure CP with zero dynamic heap allocation:
// - Skips transform on empty input or top-level `return`
// - Upgrades wrapper to `async` IIFE whenever top-level `await` or `import`
//   statements are present
// - Hoists `var`/`let`/`const`/`function`/`class`/`import` bindings into
//   deduplicated top-level `var` declarations for cross-line REPL persistence
// - Marks trailing expression statements for `{ value: <expr> }` result capture.

pub const REPL_MAX_STMTS: usize = 32;
pub const REPL_MAX_HOISTED_VARS: usize = 64;

pub err ReplTransformError {
    TooManyStmts,
    TooManyHoistedVars,
    BufferTooSmall,
}

/// Statement category recognized by the REPL transform planner.
pub enum ReplStmtKind: u8 {
    expr_stmt = 0,
    var_decl = 1,
    let_decl = 2,
    const_decl = 3,
    fn_decl = 4,
    class_decl = 5,
    import_stmt = 6,
    other_stmt = 7,

    fn hoistsBinding(): bool {
        return _ == ReplStmtKind.var_decl ||
            _ == ReplStmtKind.let_decl ||
            _ == ReplStmtKind.const_decl ||
            _ == ReplStmtKind.fn_decl ||
            _ == ReplStmtKind.class_decl ||
            _ == ReplStmtKind.import_stmt;
    }

    fn requiresAsync(): bool {
        return _ == ReplStmtKind.import_stmt;
    }
}

/// Wrapper mode chosen for REPL evaluation.
pub enum ReplWrapMode: u8 {
    skip = 0,
    sync_iife = 1,
    async_iife = 2,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): ReplTransformError!usize {
    if (src.len > out.len - pos) return ReplTransformError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// Lightweight descriptor for a top-level statement submitted to the REPL.
pub struct ReplStmtDesc {
    kind: ReplStmtKind = ReplStmtKind.other_stmt,
    binding_name: []const u8 = "",
    has_initializer: bool = false,
    triggers_await: bool = false,

    static fn expr(triggers_await: bool): ReplStmtDesc {
        return ReplStmtDesc{
            kind: ReplStmtKind.expr_stmt,
            binding_name: "",
            has_initializer: false,
            triggers_await: triggers_await,
        };
    }

    static fn decl(kind: ReplStmtKind, name: []const u8, has_init: bool): ReplStmtDesc {
        return ReplStmtDesc{
            kind: kind,
            binding_name: name,
            has_initializer: has_init,
            triggers_await: false,
        };
    }

    static fn importDecl(name: []const u8): ReplStmtDesc {
        return ReplStmtDesc{
            kind: ReplStmtKind.import_stmt,
            binding_name: name,
            has_initializer: true,
            triggers_await: true,
        };
    }
}

/// Top-level REPL input program summary.
pub struct ReplProgramInput {
    has_top_level_return: bool = false,
    has_top_level_await: bool = false,
    stmts: [REPL_MAX_STMTS]ReplStmtDesc = [ReplStmtDesc{}; REPL_MAX_STMTS],
    stmt_count: usize = 0,

    static fn init(): ReplProgramInput {
        return ReplProgramInput{};
    }

    fn addStmt(*_, stmt: ReplStmtDesc): ReplTransformError!usize {
        if (_.stmt_count >= REPL_MAX_STMTS) return ReplTransformError.TooManyStmts;
        const idx = _.stmt_count;
        _.stmts[idx] = stmt;
        _.stmt_count += 1;
        return idx;
    }
}

/// Zero-allocation REPL transformation plan produced by `planReplTransform`.
pub struct ReplTransformPlan {
    mode: ReplWrapMode = ReplWrapMode.skip,
    hoisted_vars: [REPL_MAX_HOISTED_VARS][]const u8 = [""; REPL_MAX_HOISTED_VARS],
    hoisted_count: usize = 0,
    captures_last_expr: bool = false,
    last_expr_stmt_index: usize? = null,
    inner_stmt_count: usize = 0,

    fn hasHoistedVar(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.hoisted_count) {
            if (strEq(_.hoisted_vars[i], name)) return true;
            i += 1;
        }
        return false;
    }

    fn pushHoisted(*_, name: []const u8): ReplTransformError!bool {
        if (name.len == 0) return false;
        if (_.hasHoistedVar(name)) return false;
        if (_.hoisted_count >= REPL_MAX_HOISTED_VARS) {
            return ReplTransformError.TooManyHoistedVars;
        }
        _.hoisted_vars[_.hoisted_count] = name;
        _.hoisted_count += 1;
        return true;
    }

    /// Formats deduplicated hoisted top-level declarations (e.g. `"var x, y;\n"`).
    /// Returns 0 if there are no hoisted bindings.
    fn formatHoistedDecls(out: []u8): ReplTransformError!usize {
        if (_.hoisted_count == 0) return 0;
        let pos = appendSlice(out, 0, "var ")?;
        let i: usize = 0;
        while (i < _.hoisted_count) {
            if (i > 0) {
                pos = appendSlice(out, pos, ", ")?;
            }
            pos = appendSlice(out, pos, _.hoisted_vars[i])?;
            i += 1;
        }
        pos = appendSlice(out, pos, ";\n")?;
        return pos;
    }

    /// Formats the hoisted `var` preamble + IIFE opening prefix (`(() => {\n` or `(async () => {\n`).
    fn formatWrapperPrefix(out: []u8): ReplTransformError!usize {
        if (_.mode == ReplWrapMode.skip) return 0;
        let pos = _.formatHoistedDecls(out)?;
        if (_.mode == ReplWrapMode.async_iife) {
            pos = appendSlice(out, pos, "(async () => {\n")?;
        } else {
            pos = appendSlice(out, pos, "(() => {\n")?;
        }
        return pos;
    }

    /// Formats the IIFE closing suffix (`})()`).
    fn formatWrapperSuffix(out: []u8): ReplTransformError!usize {
        if (_.mode == ReplWrapMode.skip) return 0;
        return appendSlice(out, 0, "})()");
    }
}

/// Computes the REPL transform plan (skip vs sync IIFE vs async IIFE, deduplicated
/// hoisted `var` bindings, and last-expression `{ value: expr }` capture).
pub fn planReplTransform(input: ReplProgramInput): ReplTransformError!ReplTransformPlan {
    let plan = ReplTransformPlan{};

    if (input.has_top_level_return || input.stmt_count == 0) {
        plan.mode = ReplWrapMode.skip;
        return plan;
    }

    let is_async = input.has_top_level_await;
    let i: usize = 0;
    while (i < input.stmt_count) {
        const st = input.stmts[i];
        if (st.triggers_await || st.kind.requiresAsync()) {
            is_async = true;
        }
        if (st.kind.hoistsBinding() && st.binding_name.len > 0) {
            * = plan.pushHoisted(st.binding_name)?;
        }
        // Declarations without initializers only hoist; all others produce an inner statement
        if (st.kind == ReplStmtKind.var_decl ||
            st.kind == ReplStmtKind.let_decl ||
            st.kind == ReplStmtKind.const_decl)
        {
            if (st.has_initializer) {
                plan.inner_stmt_count += 1;
            }
        } else {
            plan.inner_stmt_count += 1;
        }
        i += 1;
    }

    plan.mode = if (is_async) ReplWrapMode.async_iife else ReplWrapMode.sync_iife;

    const last_idx = input.stmt_count - 1;
    if (input.stmts[last_idx].kind == ReplStmtKind.expr_stmt) {
        plan.captures_last_expr = true;
        plan.last_expr_stmt_index = last_idx;
    }

    return plan;
}
