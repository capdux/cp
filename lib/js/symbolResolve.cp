// Source: Absorbed from /data/dev/bun/src/js_parser/ast/symbols.cp | Synced: 2026-11-18T22:40:30Z
// ============================================================================
// lib/js/symbolResolve.cp — Zero-Alloc JS/TS Lexical Symbol Resolver
// ============================================================================
// Absorbed from Bun `js_parser/ast/symbols.cp`. Implements single-hash lexical
// scope-chain symbol lookup with:
// - `with`-scope traversal tracking (`must_not_be_renamed` minifier protection)
// - Class-body `forbid_arguments` diagnostic flagging
// - TypeScript `namespace` / `enum` exported member lazy property-access aliasing
// - Unbound global symbol auto-materialization in module scope & usage counting
// Backed entirely by caller-provided external slices (no hardcoded small limits).

pub const JS_RESOLVE_NONE: u32 = 0xFFFFFFFF;
pub const JS_RESOLVE_MAX_DEPTH: u32 = 256;

pub err JsSymbolResolveError {
    TableFull,
    InvalidScope,
    InvalidSymbol,
    EmptyName,
    CycleDetected,
}

/// Kind of scope traversed during lexical symbol resolution.
pub enum JsResolveScopeKind: u8 {
    module_root = 0,
    block = 1,
    function_scope = 2,
    class_body = 3,
    with_scope = 4,
    ts_namespace = 5,
    ts_enum = 6,
}

/// Kind of symbol recorded in the resolver symbol table.
pub enum JsResolveSymKind: u8 {
    declared = 0,
    unbound = 1,
    ts_namespace_alias = 2,
}

/// FNV-1a 64-bit hash for single-pass scope-chain member lookup.
pub fn hashJsSymbolName(name: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < name.len) {
        h ^= @as(u64, name[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Resolved symbol record stored in external `[]JsResolveSymbol` storage.
pub struct JsResolveSymbol {
    name: []const u8 = "",
    kind: JsResolveSymKind = JsResolveSymKind.declared,
    declare_loc: u32 = 0,
    use_count: u32 = 0,
    must_not_be_renamed: bool = false,
    /// For `ts_namespace_alias`: target namespace argument symbol id.
    namespace_ref: u32 = JS_RESOLVE_NONE,

    static fn empty(): JsResolveSymbol {
        return JsResolveSymbol{};
    }
}

/// Scope descriptor stored in external `[]JsResolveScope` storage.
pub struct JsResolveScope {
    kind: JsResolveScopeKind = JsResolveScopeKind.block,
    parent: u32 = JS_RESOLVE_NONE,
    forbid_arguments: bool = false,
    /// For `ts_namespace` / `ts_enum` scopes: namespace argument symbol id.
    ts_namespace_arg_ref: u32 = JS_RESOLVE_NONE,
    is_ts_enum_scope: bool = false,

    static fn empty(): JsResolveScope {
        return JsResolveScope{};
    }
}

/// Lexical scope binding entry stored in external `[]JsResolveMember` storage.
pub struct JsResolveMember {
    scope_id: u32 = JS_RESOLVE_NONE,
    name: []const u8 = "",
    name_hash: u64 = 0,
    sym_id: u32 = JS_RESOLVE_NONE,
    loc: u32 = 0,

    static fn empty(): JsResolveMember {
        return JsResolveMember{};
    }
}

/// Exported member of a TypeScript `namespace` or `enum` scope.
pub struct JsTsNsExport {
    scope_id: u32 = JS_RESOLVE_NONE,
    name: []const u8 = "",
    name_hash: u64 = 0,
    loc: u32 = 0,
    is_enum_member: bool = false,
    /// Lazily populated alias symbol id on first access.
    cached_alias_sym: u32 = JS_RESOLVE_NONE,

    static fn empty(): JsTsNsExport {
        return JsTsNsExport{};
    }
}

/// Result of `findSymbol` / `findSymbolProbe` (mirrors Bun `FindSymbolResult`).
pub struct JsFindSymbolResult {
    sym_id: u32 = JS_RESOLVE_NONE,
    declare_loc: u32 = 0,
    is_inside_with_scope: bool = false,
    forbid_arguments_error: bool = false,
    created_unbound: bool = false,
    resolved_via_ts_namespace: bool = false,
}

/// External-slice-driven JS/TS lexical symbol resolver.
pub struct JsSymbolResolver {
    scopes: []JsResolveScope,
    members: []JsResolveMember,
    symbols: []JsResolveSymbol,
    ts_exports: []JsTsNsExport,
    scope_len: usize = 0,
    member_len: usize = 0,
    symbol_len: usize = 0,
    ts_export_len: usize = 0,
    module_scope_id: u32 = JS_RESOLVE_NONE,

    static fn init(
        scopes: []JsResolveScope,
        members: []JsResolveMember,
        symbols: []JsResolveSymbol,
        ts_exports: []JsTsNsExport,
    ): JsSymbolResolveError!JsSymbolResolver {
        if (scopes.len == 0) return JsSymbolResolveError.TableFull;
        scopes[0] = JsResolveScope{
            kind: JsResolveScopeKind.module_root,
            parent: JS_RESOLVE_NONE,
            forbid_arguments: false,
            ts_namespace_arg_ref: JS_RESOLVE_NONE,
            is_ts_enum_scope: false,
        };
        return JsSymbolResolver{
            scopes: scopes,
            members: members,
            symbols: symbols,
            ts_exports: ts_exports,
            scope_len: 1,
            member_len: 0,
            symbol_len: 0,
            ts_export_len: 0,
            module_scope_id: 0,
        };
    }

    fn moduleScope(): u32 {
        return _.module_scope_id;
    }

    fn symbolCount(): usize {
        return _.symbol_len;
    }

    fn scopeCount(): usize {
        return _.scope_len;
    }

    fn getSymbol(sym_id: u32): JsSymbolResolveError!JsResolveSymbol {
        const idx: usize = @as(usize, sym_id);
        if (idx >= _.symbol_len) return JsSymbolResolveError.InvalidSymbol;
        return _.symbols[idx];
    }

    /// Pushes a child lexical scope under `parent_scope`.
    fn pushScope(*_, parent_scope: u32, kind: JsResolveScopeKind): JsSymbolResolveError!u32 {
        if (@as(usize, parent_scope) >= _.scope_len) return JsSymbolResolveError.InvalidScope;
        if (_.scope_len >= _.scopes.len) return JsSymbolResolveError.TableFull;
        const id: u32 = @as(u32, @intCast(_.scope_len));
        _.scopes[_.scope_len] = JsResolveScope{
            kind: kind,
            parent: parent_scope,
            forbid_arguments: kind == JsResolveScopeKind.class_body,
            ts_namespace_arg_ref: JS_RESOLVE_NONE,
            is_ts_enum_scope: kind == JsResolveScopeKind.ts_enum,
        };
        _.scope_len += 1;
        return id;
    }

    /// Attaches a TypeScript namespace/enum runtime argument symbol to `scope_id`.
    fn attachTsNamespace(*_, scope_id: u32, arg_sym_id: u32, is_enum: bool): JsSymbolResolveError!void {
        if (@as(usize, scope_id) >= _.scope_len) return JsSymbolResolveError.InvalidScope;
        if (@as(usize, arg_sym_id) >= _.symbol_len) return JsSymbolResolveError.InvalidSymbol;
        _.scopes[@as(usize, scope_id)].ts_namespace_arg_ref = arg_sym_id;
        _.scopes[@as(usize, scope_id)].is_ts_enum_scope = is_enum;
    }

    fn allocSymbol(*_, name: []const u8, kind: JsResolveSymKind, loc: u32, ns_ref: u32): JsSymbolResolveError!u32 {
        if (_.symbol_len >= _.symbols.len) return JsSymbolResolveError.TableFull;
        const id: u32 = @as(u32, @intCast(_.symbol_len));
        _.symbols[_.symbol_len] = JsResolveSymbol{
            name: name,
            kind: kind,
            declare_loc: loc,
            use_count: 0,
            must_not_be_renamed: false,
            namespace_ref: ns_ref,
        };
        _.symbol_len += 1;
        return id;
    }

    /// Declares a lexical symbol inside `scope_id` and returns its `sym_id`.
    fn declareSymbol(*_, scope_id: u32, name: []const u8, loc: u32): JsSymbolResolveError!u32 {
        if (name.len == 0) return JsSymbolResolveError.EmptyName;
        if (@as(usize, scope_id) >= _.scope_len) return JsSymbolResolveError.InvalidScope;
        if (_.member_len >= _.members.len) return JsSymbolResolveError.TableFull;

        const sym_id = _.allocSymbol(name, JsResolveSymKind.declared, loc, JS_RESOLVE_NONE)?;
        _.members[_.member_len] = JsResolveMember{
            scope_id: scope_id,
            name: name,
            name_hash: hashJsSymbolName(name),
            sym_id: sym_id,
            loc: loc,
        };
        _.member_len += 1;
        return sym_id;
    }

    /// Registers an exported member on a TypeScript `namespace` or `enum` scope.
    fn addTsNamespaceExport(*_, scope_id: u32, name: []const u8, loc: u32, is_enum_member: bool): JsSymbolResolveError!void {
        if (name.len == 0) return JsSymbolResolveError.EmptyName;
        if (@as(usize, scope_id) >= _.scope_len) return JsSymbolResolveError.InvalidScope;
        if (_.ts_export_len >= _.ts_exports.len) return JsSymbolResolveError.TableFull;
        _.ts_exports[_.ts_export_len] = JsTsNsExport{
            scope_id: scope_id,
            name: name,
            name_hash: hashJsSymbolName(name),
            loc: loc,
            is_enum_member: is_enum_member,
            cached_alias_sym: JS_RESOLVE_NONE,
        };
        _.ts_export_len += 1;
    }

    fn findMemberInScope(scope_id: u32, name: []const u8, name_hash: u64): usize? {
        let i: usize = _.member_len;
        while (i > 0) {
            i -= 1;
            const m = _.members[i];
            if (m.scope_id == scope_id && m.name_hash == name_hash && strEql(m.name, name)) {
                return i;
            }
        }
        return null;
    }

    fn findTsExportInScope(scope_id: u32, name: []const u8, name_hash: u64, is_enum_scope: bool): usize? {
        let i: usize = 0;
        while (i < _.ts_export_len) {
            const ex = _.ts_exports[i];
            if (ex.scope_id == scope_id && ex.is_enum_member == is_enum_scope && ex.name_hash == name_hash && strEql(ex.name, name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Resolves `name` starting at `start_scope` (mirrors Bun `findSymbolWithRecordUsage`).
    /// When `record_usage == true`, increments `use_count` and materializes unbound symbols in `module_root`.
    fn findSymbolWithRecordUsage(*_, start_scope: u32, loc: u32, name: []const u8, record_usage: bool): JsSymbolResolveError!JsFindSymbolResult {
        if (name.len == 0) return JsSymbolResolveError.EmptyName;
        if (@as(usize, start_scope) >= _.scope_len) return JsSymbolResolveError.InvalidScope;

        const name_hash = hashJsSymbolName(name);
        const is_arguments_ident = strEql(name, "arguments");

        let is_inside_with_scope = false;
        let forbid_arguments_error = false;
        let current: u32 = start_scope;
        let steps: u32 = 0;

        let found_sym: u32 = JS_RESOLVE_NONE;
        let declare_loc: u32 = loc;
        let resolved_via_ts = false;
        let created_unbound = false;

        while (current != JS_RESOLVE_NONE) {
            if (steps >= JS_RESOLVE_MAX_DEPTH) return JsSymbolResolveError.CycleDetected;
            steps += 1;

            const sc = _.scopes[@as(usize, current)];
            if (sc.kind == JsResolveScopeKind.with_scope) {
                is_inside_with_scope = true;
            }
            if (sc.forbid_arguments && is_arguments_ident) {
                forbid_arguments_error = true;
            }

            if (m_idx := _.findMemberInScope(current, name, name_hash)) {
                const mem = _.members[m_idx];
                found_sym = mem.sym_id;
                declare_loc = mem.loc;
                break;
            }

            if (sc.ts_namespace_arg_ref != JS_RESOLVE_NONE) {
                if (ex_idx := _.findTsExportInScope(current, name, name_hash, sc.is_ts_enum_scope)) {
                    declare_loc = _.ts_exports[ex_idx].loc;
                    resolved_via_ts = true;
                    if (_.ts_exports[ex_idx].cached_alias_sym != JS_RESOLVE_NONE) {
                        found_sym = _.ts_exports[ex_idx].cached_alias_sym;
                    } else {
                        const alias_sym = _.allocSymbol(
                            name,
                            JsResolveSymKind.ts_namespace_alias,
                            declare_loc,
                            sc.ts_namespace_arg_ref,
                        )?;
                        _.ts_exports[ex_idx].cached_alias_sym = alias_sym;
                        found_sym = alias_sym;
                    }
                    break;
                }
            }

            current = sc.parent;
        }

        if (found_sym == JS_RESOLVE_NONE) {
            if (!record_usage) {
                return JsFindSymbolResult{
                    sym_id: JS_RESOLVE_NONE,
                    declare_loc: loc,
                    is_inside_with_scope: is_inside_with_scope,
                    forbid_arguments_error: forbid_arguments_error,
                    created_unbound: false,
                    resolved_via_ts_namespace: false,
                };
            }
            if (_.member_len >= _.members.len) return JsSymbolResolveError.TableFull;
            const unbound_sym = _.allocSymbol(name, JsResolveSymKind.unbound, loc, JS_RESOLVE_NONE)?;
            _.members[_.member_len] = JsResolveMember{
                scope_id: _.module_scope_id,
                name: name,
                name_hash: name_hash,
                sym_id: unbound_sym,
                loc: loc,
            };
            _.member_len += 1;
            found_sym = unbound_sym;
            declare_loc = loc;
            created_unbound = true;
        }

        if (is_inside_with_scope) {
            _.symbols[@as(usize, found_sym)].must_not_be_renamed = true;
        }
        if (record_usage) {
            _.symbols[@as(usize, found_sym)].use_count += 1;
        }

        return JsFindSymbolResult{
            sym_id: found_sym,
            declare_loc: declare_loc,
            is_inside_with_scope: is_inside_with_scope,
            forbid_arguments_error: forbid_arguments_error,
            created_unbound: created_unbound,
            resolved_via_ts_namespace: resolved_via_ts,
        };
    }

    /// Standard lookup that records usage and materializes unbound symbols (mirrors Bun `findSymbol`).
    fn findSymbol(*_, start_scope: u32, loc: u32, name: []const u8): JsSymbolResolveError!JsFindSymbolResult {
        return _.findSymbolWithRecordUsage(start_scope, loc, name, true);
    }

    /// Non-mutating probe lookup (does not allocate unbound symbols or increment `use_count`).
    fn probeSymbol(*_, start_scope: u32, loc: u32, name: []const u8): JsSymbolResolveError!JsFindSymbolResult {
        return _.findSymbolWithRecordUsage(start_scope, loc, name, false);
    }
}
