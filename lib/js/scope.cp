// Source: Absorbed from /data/dev/bun/src/js_parser/ast/Scope.cp | Synced: 2026-11-22T19:00:32Z
// ============================================================================
// lib/js/scope.cp — Zero-Alloc JS/TS Lexical Scope Tree & Symbol Merge Engine
// ============================================================================
// Implements Bun's `js_parser/ast/Scope.cp` lexical scope hierarchy, ECMAScript
// & TypeScript symbol declaration merge rules (`canMergeJsSymbols`), `var`
// hoisting boundary resolution (`stopsHoisting`), strict-mode propagation,
// and direct-`eval()` minification-safety tracking with zero heap allocation.

pub const JS_SCOPE_MAX_SCOPES: usize = 64;
pub const JS_SCOPE_MAX_MEMBERS: usize = 256;
pub const JS_SCOPE_NO_PARENT: u32 = 0xFFFFFFFF;

pub err JsScopeError {
    ScopesFull,
    MembersFull,
    InvalidScopeId,
    MergeForbidden,
}

/// Lexical scope category (aligned with Bun's `Scope.Kind`).
/// Kinds at or after `entry` stop hoisted `var` declarations from escaping.
pub enum JsScopeKind: u8 {
    block = 0,
    with_scope = 1,
    label = 2,
    class_name = 3,
    class_body = 4,
    catch_binding = 5,
    entry = 6,
    function_args = 7,
    function_body = 8,
    class_static_init = 9,

    fn stopsHoisting(): bool {
        return @intFromEnum(_) >= @intFromEnum(JsScopeKind.entry);
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .block => "block",
            .with_scope => "with",
            .label => "label",
            .class_name => "class_name",
            .class_body => "class_body",
            .catch_binding => "catch_binding",
            .entry => "entry",
            .function_args => "function_args",
            .function_body => "function_body",
            .class_static_init => "class_static_init",
        };
    }
}

/// Strict-mode state for a lexical scope.
pub enum JsStrictModeKind: u8 {
    sloppy_mode = 0,
    explicit_strict_mode = 1,
    implicit_strict_mode_import = 2,
    implicit_strict_mode_export = 3,
    implicit_strict_mode_class = 4,

    fn isStrict(): bool {
        return _ != JsStrictModeKind.sloppy_mode;
    }
}

/// Declaration symbol kind participating in scope binding & merge checks.
pub enum JsSymbolKind: u8 {
    unbound = 0,
    import_sym = 1,
    hoisted = 2,
    hoisted_function = 3,
    generator_or_async_function = 4,
    let_sym = 5,
    const_sym = 6,
    class_sym = 7,
    catch_identifier = 8,
    arguments_sym = 9,
    ts_enum = 10,
    ts_namespace = 11,
    private_get = 12,
    private_set = 13,
    private_get_set_pair = 14,
    private_static_get = 15,
    private_static_set = 16,
    private_static_get_set_pair = 17,

    fn isHoisted(): bool {
        return _ == JsSymbolKind.hoisted || _ == JsSymbolKind.hoisted_function;
    }

    fn isHoistedOrFunction(): bool {
        return _ == JsSymbolKind.hoisted ||
            _ == JsSymbolKind.hoisted_function ||
            _ == JsSymbolKind.generator_or_async_function;
    }
}

/// Result of attempting to merge a newly declared symbol with an existing binding
/// in the same lexical scope (aligned with Bun's `Scope.SymbolMergeResult`).
pub enum JsSymbolMergeResult: u8 {
    forbidden = 0,
    replace_with_new = 1,
    overwrite_with_new = 2,
    keep_existing = 3,
    become_private_get_set_pair = 4,
    become_private_static_get_set_pair = 5,
}

/// Evaluates ECMAScript + TypeScript symbol merge rules (1:1 port of Bun's `Scope.canMergeSymbols`).
pub fn canMergeJsSymbols(
    scope_kind: JsScopeKind,
    existing: JsSymbolKind,
    new_kind: JsSymbolKind,
    is_typescript: bool,
): JsSymbolMergeResult {
    if (existing == JsSymbolKind.unbound) {
        return JsSymbolMergeResult.replace_with_new;
    }

    if (is_typescript) {
        // In TypeScript, imports may be type-only and silently collide with module bindings.
        if (existing == JsSymbolKind.import_sym) {
            return JsSymbolMergeResult.replace_with_new;
        }

        // `enum Foo {} enum Foo {}` / `namespace Foo {} enum Foo {}`
        if (new_kind == JsSymbolKind.ts_enum &&
            (existing == JsSymbolKind.ts_enum || existing == JsSymbolKind.ts_namespace))
        {
            return JsSymbolMergeResult.replace_with_new;
        }

        // `namespace Foo {}` merging with namespace / enum / function / class
        if (new_kind == JsSymbolKind.ts_namespace) {
            if (existing == JsSymbolKind.ts_namespace ||
                existing == JsSymbolKind.ts_enum ||
                existing == JsSymbolKind.hoisted_function ||
                existing == JsSymbolKind.generator_or_async_function ||
                existing == JsSymbolKind.class_sym)
            {
                return JsSymbolMergeResult.keep_existing;
            }
        }
    }

    // `var foo; var foo;`, `var foo; function foo() {}`,
    // `function *foo() {} function *foo() {}` (at entry/function scope, not block scope)
    if (new_kind.isHoistedOrFunction() &&
        existing.isHoistedOrFunction() &&
        (scope_kind == JsScopeKind.entry ||
            scope_kind == JsScopeKind.function_body ||
            scope_kind == JsScopeKind.function_args ||
            (new_kind == existing && existing.isHoisted())))
    {
        return JsSymbolMergeResult.replace_with_new;
    }

    // `get #foo() {} set #foo() {}`
    if ((existing == JsSymbolKind.private_get && new_kind == JsSymbolKind.private_set) ||
        (existing == JsSymbolKind.private_set && new_kind == JsSymbolKind.private_get))
    {
        return JsSymbolMergeResult.become_private_get_set_pair;
    }
    if ((existing == JsSymbolKind.private_static_get && new_kind == JsSymbolKind.private_static_set) ||
        (existing == JsSymbolKind.private_static_set && new_kind == JsSymbolKind.private_static_get))
    {
        return JsSymbolMergeResult.become_private_static_get_set_pair;
    }

    // `try {} catch (e) { var e }`
    if (existing == JsSymbolKind.catch_identifier && new_kind == JsSymbolKind.hoisted) {
        return JsSymbolMergeResult.replace_with_new;
    }

    // `function() { var arguments }`
    if (existing == JsSymbolKind.arguments_sym && new_kind == JsSymbolKind.hoisted) {
        return JsSymbolMergeResult.keep_existing;
    }

    // `function() { let arguments }`
    if (existing == JsSymbolKind.arguments_sym && new_kind != JsSymbolKind.hoisted) {
        return JsSymbolMergeResult.overwrite_with_new;
    }

    return JsSymbolMergeResult.forbidden;
}

fn hashName(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h = h ^ @as(u64, s[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
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

/// Single bound identifier inside a lexical scope.
pub struct JsScopeMember {
    in_use: bool = false,
    scope_id: u32 = 0,
    name_hash: u64 = 0,
    name: []const u8 = "",
    sym_kind: JsSymbolKind = JsSymbolKind.unbound,
    ref_id: u32 = 0,
    loc_start: u32 = 0,
}

/// Lexical scope node stored inside `JsScopeTree`.
pub struct JsScopeNode {
    in_use: bool = false,
    kind: JsScopeKind = JsScopeKind.block,
    parent_id: u32 = JS_SCOPE_NO_PARENT,
    strict_mode: JsStrictModeKind = JsStrictModeKind.sloppy_mode,
    contains_direct_eval: bool = false,
    forbid_arguments: bool = false,
    label_stmt_is_loop: bool = false,
}

/// Fixed-capacity zero-allocation JS/TS lexical scope tree.
pub struct JsScopeTree {
    is_typescript: bool = false,
    scopes: [JS_SCOPE_MAX_SCOPES]JsScopeNode = [JsScopeNode{}; JS_SCOPE_MAX_SCOPES],
    scope_count: usize = 0,
    members: [JS_SCOPE_MAX_MEMBERS]JsScopeMember = [JsScopeMember{}; JS_SCOPE_MAX_MEMBERS],
    member_count: usize = 0,

    static fn init(is_typescript: bool): JsScopeTree {
        return JsScopeTree{ is_typescript: is_typescript };
    }

    fn scopeCount(): usize {
        return _.scope_count;
    }

    fn memberCount(): usize {
        return _.member_count;
    }

    /// Creates a new scope node under optional `parent_id`, inheriting strict mode and
    /// `forbid_arguments` from parent (class bodies implicitly enable strict mode).
    fn pushScope(*_, kind: JsScopeKind, parent_id: u32?): JsScopeError!u32 {
        if (_.scope_count >= JS_SCOPE_MAX_SCOPES) return JsScopeError.ScopesFull;

        let p_raw = JS_SCOPE_NO_PARENT;
        let strict = JsStrictModeKind.sloppy_mode;
        let forbid_args = false;

        if (pid := parent_id) {
            const pidx = @as(usize, pid);
            if (pidx >= _.scope_count || !_.scopes[pidx].in_use) {
                return JsScopeError.InvalidScopeId;
            }
            p_raw = pid;
            strict = _.scopes[pidx].strict_mode;
            forbid_args = _.scopes[pidx].forbid_arguments;
        }

        if (kind == JsScopeKind.class_body) {
            if (!strict.isStrict()) strict = JsStrictModeKind.implicit_strict_mode_class;
            forbid_args = true;
        }

        const id = @as(u32, @intCast(_.scope_count));
        _.scopes[_.scope_count] = JsScopeNode{
            in_use: true,
            kind: kind,
            parent_id: p_raw,
            strict_mode: strict,
            contains_direct_eval: false,
            forbid_arguments: forbid_args,
            label_stmt_is_loop: false,
        };
        _.scope_count += 1;
        return id;
    }

    fn getScope(scope_id: u32): JsScopeNode? {
        const idx = @as(usize, scope_id);
        if (idx >= _.scope_count || !_.scopes[idx].in_use) return null;
        return _.scopes[idx];
    }

    /// Declares or merges a symbol in `scope_id`. Returns `MergeForbidden` on illegal redeclaration.
    fn declareSymbol(
        *_,
        scope_id: u32,
        name: []const u8,
        sym_kind: JsSymbolKind,
        ref_id: u32,
        loc_start: u32,
    ): JsScopeError!JsSymbolMergeResult {
        const sidx = @as(usize, scope_id);
        if (sidx >= _.scope_count || !_.scopes[sidx].in_use) {
            return JsScopeError.InvalidScopeId;
        }
        const h = hashName(name);

        let i: usize = 0;
        while (i < _.member_count) {
            if (_.members[i].in_use &&
                _.members[i].scope_id == scope_id &&
                _.members[i].name_hash == h &&
                strEq(_.members[i].name, name))
            {
                const res = canMergeJsSymbols(_.scopes[sidx].kind, _.members[i].sym_kind, sym_kind, _.is_typescript);
                switch (res) {
                    .forbidden => return JsScopeError.MergeForbidden,
                    .replace_with_new, .overwrite_with_new => {
                        _.members[i].sym_kind = sym_kind;
                        _.members[i].ref_id = ref_id;
                        _.members[i].loc_start = loc_start;
                    },
                    .keep_existing => {},
                    .become_private_get_set_pair => {
                        _.members[i].sym_kind = JsSymbolKind.private_get_set_pair;
                    },
                    .become_private_static_get_set_pair => {
                        _.members[i].sym_kind = JsSymbolKind.private_static_get_set_pair;
                    },
                }
                return res;
            }
            i += 1;
        }

        if (_.member_count >= JS_SCOPE_MAX_MEMBERS) return JsScopeError.MembersFull;
        _.members[_.member_count] = JsScopeMember{
            in_use: true,
            scope_id: scope_id,
            name_hash: h,
            name: name,
            sym_kind: sym_kind,
            ref_id: ref_id,
            loc_start: loc_start,
        };
        _.member_count += 1;
        return JsSymbolMergeResult.replace_with_new;
    }

    fn lookupInScope(scope_id: u32, name: []const u8): JsScopeMember? {
        const h = hashName(name);
        let i: usize = 0;
        while (i < _.member_count) {
            if (_.members[i].in_use &&
                _.members[i].scope_id == scope_id &&
                _.members[i].name_hash == h &&
                strEq(_.members[i].name, name))
            {
                return _.members[i];
            }
            i += 1;
        }
        return null;
    }

    /// Walks the lexical parent chain to resolve `name`.
    fn resolveSymbol(scope_id: u32, name: []const u8): JsScopeMember? {
        let cur = scope_id;
        let steps: usize = 0;
        while (cur != JS_SCOPE_NO_PARENT && steps < JS_SCOPE_MAX_SCOPES) {
            const idx = @as(usize, cur);
            if (idx >= _.scope_count || !_.scopes[idx].in_use) return null;
            if (m := _.lookupInScope(cur, name)) return m;
            cur = _.scopes[idx].parent_id;
            steps += 1;
        }
        return null;
    }

    /// Finds the nearest enclosing scope where `var` hoisting stops (`kind.stopsHoisting()`).
    fn findHoistTargetScope(scope_id: u32): u32? {
        let cur = scope_id;
        let steps: usize = 0;
        while (cur != JS_SCOPE_NO_PARENT && steps < JS_SCOPE_MAX_SCOPES) {
            const idx = @as(usize, cur);
            if (idx >= _.scope_count || !_.scopes[idx].in_use) return null;
            if (_.scopes[idx].kind.stopsHoisting()) return cur;
            cur = _.scopes[idx].parent_id;
            steps += 1;
        }
        return null;
    }

    /// Marks `scope_id` and all ancestor scopes as tainted by direct `eval()`,
    /// disabling identifier minification along the chain (aligned with Bun's Scope invariant).
    fn markDirectEval(*_, scope_id: u32): JsScopeError!usize {
        const sidx = @as(usize, scope_id);
        if (sidx >= _.scope_count || !_.scopes[sidx].in_use) {
            return JsScopeError.InvalidScopeId;
        }
        let cur = scope_id;
        let marked: usize = 0;
        let steps: usize = 0;
        while (cur != JS_SCOPE_NO_PARENT && steps < JS_SCOPE_MAX_SCOPES) {
            const idx = @as(usize, cur);
            if (idx >= _.scope_count || !_.scopes[idx].in_use) break;
            if (!_.scopes[idx].contains_direct_eval) {
                _.scopes[idx].contains_direct_eval = true;
                marked += 1;
            }
            cur = _.scopes[idx].parent_id;
            steps += 1;
        }
        return marked;
    }

    fn canMinifyIdentifiersInScope(scope_id: u32): bool {
        const idx = @as(usize, scope_id);
        if (idx >= _.scope_count || !_.scopes[idx].in_use) return false;
        return !_.scopes[idx].contains_direct_eval;
    }

    /// Iteratively propagates `mode` to `scope_id` and all descendant `sloppy_mode` scopes
    /// (zero-recursion port of Bun's `Scope.recursiveSetStrictMode`).
    fn recursiveSetStrictMode(*_, scope_id: u32, mode: JsStrictModeKind): JsScopeError!usize {
        const root_idx = @as(usize, scope_id);
        if (root_idx >= _.scope_count || !_.scopes[root_idx].in_use) {
            return JsScopeError.InvalidScopeId;
        }
        if (_.scopes[root_idx].strict_mode != JsStrictModeKind.sloppy_mode) return 0;

        let queue: [JS_SCOPE_MAX_SCOPES]u32 = [0; JS_SCOPE_MAX_SCOPES];
        let head: usize = 0;
        let tail: usize = 0;

        _.scopes[root_idx].strict_mode = mode;
        queue[tail] = scope_id;
        tail += 1;
        let updated: usize = 1;

        while (head < tail) {
            const parent = queue[head];
            head += 1;
            let i: usize = 0;
            while (i < _.scope_count) {
                if (_.scopes[i].in_use &&
                    _.scopes[i].parent_id == parent &&
                    _.scopes[i].strict_mode == JsStrictModeKind.sloppy_mode)
                {
                    _.scopes[i].strict_mode = mode;
                    updated += 1;
                    if (tail < JS_SCOPE_MAX_SCOPES) {
                        queue[tail] = @as(u32, @intCast(i));
                        tail += 1;
                    }
                }
                i += 1;
            }
        }
        return updated;
    }
}
