// Source: cp/lib/graphql/parser.cp (GraphQL Executable Document AST Parser & Guardrails)
// Pure CP GraphQL October 2021 Executable Document recursive-descent AST parser,
// Arena-backed document lifetime, configurable MAX_DEPTH / MAX_NODES / MAX_FIELDS / MAX_ALIASES
// security guardrails, and selection complexity analyzer.

const std = @import("std");
import { MAX_TOKENS, GqlError, GqlTokenKind, GqlToken, GqlLexer } from "./lexer.cp";

pub const MAX_DEPTH: usize = 32;
pub const MAX_NODES: usize = 16384;
pub const MAX_FIELDS: usize = 4096;
pub const MAX_ALIASES: usize = 256;

/// Configurable parser security guardrails against deep nesting and amplification queries.
pub struct GqlParseOptions {
    max_depth: usize = MAX_DEPTH,
    max_tokens: usize = MAX_TOKENS,
    max_nodes: usize = MAX_NODES,
    max_fields: usize = MAX_FIELDS,
    max_aliases: usize = MAX_ALIASES,
}

/// GraphQL executable operation type.
pub enum GqlOpKind {
    Query,
    Mutation,
    Subscription,

    pub fn keyword(): string {
        return switch (_) {
            .Query => "query",
            .Mutation => "mutation",
            .Subscription => "subscription",
        };
    }
}

/// Key-value field inside a GraphQL input object literal.
pub struct GqlObjectField {
    name: string = "",
    value: GqlValue = .Null,
}

/// GraphQL input value AST node.
pub enum GqlValue {
    Variable: string,
    Int: i64,
    Float: f64,
    String: string,
    Bool: bool,
    Null,
    Enum: string,
    List: []GqlValue,
    Object: []GqlObjectField,

    pub fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    pub fn asVariable(): string? {
        return switch (_) {
            .Variable v => v,
            else => null,
        };
    }

    pub fn asInt(): i64? {
        return switch (_) {
            .Int n => n,
            else => null,
        };
    }

    pub fn asFloat(): f64? {
        return switch (_) {
            .Float f => f,
            else => null,
        };
    }

    pub fn asString(): string? {
        return switch (_) {
            .String s => s,
            else => null,
        };
    }

    pub fn asBool(): bool? {
        return switch (_) {
            .Bool b => b,
            else => null,
        };
    }

    pub fn asEnum(): string? {
        return switch (_) {
            .Enum e => e,
            else => null,
        };
    }

    pub fn asList(): ([]GqlValue)? {
        return switch (_) {
            .List items => items,
            else => null,
        };
    }

    pub fn asObject(): ([]GqlObjectField)? {
        return switch (_) {
            .Object fields => fields,
            else => null,
        };
    }

    /// Deep structural equality comparison between two GraphQL AST values.
    pub fn eql(other: GqlValue): bool {
        return switch (_) {
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Bool a => switch (other) {
                .Bool b => a == b,
                else => false,
            },
            .Int a => switch (other) {
                .Int b => a == b,
                else => false,
            },
            .Float a => switch (other) {
                .Float b => a == b,
                else => false,
            },
            .String a => switch (other) {
                .String b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Enum a => switch (other) {
                .Enum b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Variable a => switch (other) {
                .Variable b => std.mem.eql(u8, a, b),
                else => false,
            },
            .List a => switch (other) {
                .List b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
            .Object a => switch (other) {
                .Object b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!std.mem.eql(u8, a[i].name, b[i].name)) return false;
                        if (!a[i].value.eql(b[i].value)) return false;
                    }
                    return true;
                },
                else => false,
            },
        };
    }
}

/// GraphQL field or directive argument (`name: value`).
pub struct GqlArgument {
    name: string = "",
    value: GqlValue = .Null,

    pub fn eql(other: GqlArgument): bool {
        return std.mem.eql(u8, _.name, other.name) && _.value.eql(other.value);
    }
}

/// GraphQL directive (`@name(args...)`).
pub struct GqlDirective {
    name: string = "",
    args: []GqlArgument = [],

    pub fn eql(other: GqlDirective): bool {
        if (!std.mem.eql(u8, _.name, other.name)) return false;
        if (_.args.len != other.args.len) return false;
        let i: usize = 0;
        while (i < _.args.len) : (i += 1) {
            if (!_.args[i].eql(other.args[i])) return false;
        }
        return true;
    }
}

/// GraphQL type annotation (`Named`, `[T]`, `T!`).
pub enum GqlType {
    Named: string,
    List: []GqlType,
    NonNull: []GqlType,

    pub fn eql(other: GqlType): bool {
        return switch (_) {
            .Named a => switch (other) {
                .Named b => std.mem.eql(u8, a, b),
                else => false,
            },
            .List a => switch (other) {
                .List b => a.len == 1 && b.len == 1 && a[0].eql(b[0]),
                else => false,
            },
            .NonNull a => switch (other) {
                .NonNull b => a.len == 1 && b.len == 1 && a[0].eql(b[0]),
                else => false,
            },
        };
    }
}

/// GraphQL operation variable definition (`$name: Type = default @dir`).
pub struct GqlVarDef {
    name: string = "",
    var_type: GqlType = GqlType.Named(""),
    default_value: GqlValue? = null,
    directives: []GqlDirective = [],

    pub fn eql(other: GqlVarDef): bool {
        if (!std.mem.eql(u8, _.name, other.name)) return false;
        if (!_.var_type.eql(other.var_type)) return false;
        if (da := _.default_value) {
            if (db := other.default_value) {
                if (!da.eql(db)) return false;
            } else {
                return false;
            }
        } else {
            if (other.default_value != null) return false;
        }
        return directivesEqual(_.directives, other.directives);
    }
}

/// GraphQL field selection (`alias: name(args...) @directives { selections... }`).
pub struct GqlField {
    alias: string? = null,
    name: string = "",
    args: []GqlArgument = [],
    directives: []GqlDirective = [],
    selections: []GqlSelection = [],

    pub fn eql(other: GqlField): bool {
        if (!optStringEqual(_.alias, other.alias)) return false;
        if (!std.mem.eql(u8, _.name, other.name)) return false;
        if (!argsEqual(_.args, other.args)) return false;
        if (!directivesEqual(_.directives, other.directives)) return false;
        return selectionsEqual(_.selections, other.selections);
    }
}

/// GraphQL named fragment spread (`...FragName @directives`).
pub struct GqlFragmentSpread {
    name: string = "",
    directives: []GqlDirective = [],

    pub fn eql(other: GqlFragmentSpread): bool {
        return std.mem.eql(u8, _.name, other.name) && directivesEqual(_.directives, other.directives);
    }
}

/// GraphQL inline fragment (`... on Type @directives { selections... }`).
pub struct GqlInlineFragment {
    type_condition: string? = null,
    directives: []GqlDirective = [],
    selections: []GqlSelection = [],

    pub fn eql(other: GqlInlineFragment): bool {
        if (!optStringEqual(_.type_condition, other.type_condition)) return false;
        if (!directivesEqual(_.directives, other.directives)) return false;
        return selectionsEqual(_.selections, other.selections);
    }
}

/// A selection item within a GraphQL selection set.
pub enum GqlSelection {
    Field: GqlField,
    FragmentSpread: GqlFragmentSpread,
    InlineFragment: GqlInlineFragment,

    pub fn eql(other: GqlSelection): bool {
        return switch (_) {
            .Field a => switch (other) {
                .Field b => a.eql(b),
                else => false,
            },
            .FragmentSpread a => switch (other) {
                .FragmentSpread b => a.eql(b),
                else => false,
            },
            .InlineFragment a => switch (other) {
                .InlineFragment b => a.eql(b),
                else => false,
            },
        };
    }
}

/// GraphQL executable operation (`query`, `mutation`, `subscription`, or shorthand `{ ... }`).
pub struct GqlOperation {
    op: GqlOpKind = .Query,
    name: string? = null,
    shorthand: bool = false,
    var_defs: []GqlVarDef = [],
    directives: []GqlDirective = [],
    selections: []GqlSelection = [],

    pub fn eql(other: GqlOperation): bool {
        if (_.op != other.op) return false;
        if (_.shorthand != other.shorthand) return false;
        if (!optStringEqual(_.name, other.name)) return false;
        if (_.var_defs.len != other.var_defs.len) return false;
        let i: usize = 0;
        while (i < _.var_defs.len) : (i += 1) {
            if (!_.var_defs[i].eql(other.var_defs[i])) return false;
        }
        if (!directivesEqual(_.directives, other.directives)) return false;
        return selectionsEqual(_.selections, other.selections);
    }
}

/// GraphQL named fragment definition (`fragment Name on Type @directives { selections... }`).
pub struct GqlFragmentDef {
    name: string = "",
    type_condition: string = "",
    directives: []GqlDirective = [],
    selections: []GqlSelection = [],

    pub fn eql(other: GqlFragmentDef): bool {
        if (!std.mem.eql(u8, _.name, other.name)) return false;
        if (!std.mem.eql(u8, _.type_condition, other.type_condition)) return false;
        if (!directivesEqual(_.directives, other.directives)) return false;
        return selectionsEqual(_.selections, other.selections);
    }
}

fn optStringEqual(a: string?, b: string?): bool {
    if (sa := a) {
        if (sb := b) {
            return std.mem.eql(u8, sa, sb);
        }
        return false;
    }
    return b == null;
}

fn argsEqual(a: []GqlArgument, b: []GqlArgument): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!a[i].eql(b[i])) return false;
    }
    return true;
}

fn directivesEqual(a: []GqlDirective, b: []GqlDirective): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!a[i].eql(b[i])) return false;
    }
    return true;
}

fn selectionsEqual(a: []GqlSelection, b: []GqlSelection): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!a[i].eql(b[i])) return false;
    }
    return true;
}

/// Arena-backed GraphQL executable document AST.
pub struct GqlDocument {
    operations: []GqlOperation = [],
    fragments: []GqlFragmentDef = [],
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Releases all arena-allocated AST nodes, slices, and strings in O(1).
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.operations = [];
        _.fragments = [];
    }

    /// Finds an operation by name (or the first unnamed operation when `name == null`).
    pub fn findOperation(name: string?): GqlOperation? {
        for (op of _.operations) {
            if (target := name) {
                if (op_name := op.name) {
                    if (std.mem.eql(u8, op_name, target)) return op;
                }
            } else {
                if (op.name == null) return op;
            }
        }
        return null;
    }

    /// Finds a named fragment definition in the document.
    pub fn findFragment(name: string): GqlFragmentDef? {
        for (frag of _.fragments) {
            if (std.mem.eql(u8, frag.name, name)) return frag;
        }
        return null;
    }

    fn selectionsDepth(sels: []GqlSelection, active_mask: u64, step_budget: usize): usize {
        if (step_budget == 0) return 0;
        let max_d: usize = 0;
        for (sel of sels) {
            let d: usize = 0;
            switch (sel) {
                .Field f => {
                    if (f.selections.len == 0) {
                        d = 1;
                    } else {
                        d = 1 + _.selectionsDepth(f.selections, active_mask, step_budget - 1);
                    }
                },
                .InlineFragment inf => {
                    d = _.selectionsDepth(inf.selections, active_mask, step_budget - 1);
                },
                .FragmentSpread fs => {
                    let idx: usize = 0;
                    while (idx < _.fragments.len) : (idx += 1) {
                        if (std.mem.eql(u8, _.fragments[idx].name, fs.name)) {
                            if (idx < 64) {
                                const bit = (@as(u64, 1) << @as(u6, @intCast(idx)));
                                if ((active_mask & bit) == 0) {
                                    d = _.selectionsDepth(_.fragments[idx].selections, active_mask | bit, step_budget - 1);
                                }
                            } else {
                                d = _.selectionsDepth(_.fragments[idx].selections, active_mask, step_budget - 1);
                            }
                            break;
                        }
                    }
                },
            }
            if (d > max_d) max_d = d;
        }
        return max_d;
    }

    /// Computes the maximum nested field selection depth across all operations and fragments.
    /// Named fragment spreads are expanded with cycle detection.
    pub fn maxSelectionDepth(): usize {
        let max_d: usize = 0;
        for (op of _.operations) {
            const d = _.selectionsDepth(op.selections, 0, 2048);
            if (d > max_d) max_d = d;
        }
        let idx: usize = 0;
        while (idx < _.fragments.len) : (idx += 1) {
            const mask: u64 = if (idx < 64) (@as(u64, 1) << @as(u6, @intCast(idx))) else 0;
            const d = _.selectionsDepth(_.fragments[idx].selections, mask, 2048);
            if (d > max_d) max_d = d;
        }
        return max_d;
    }

    fn countSelectionFields(sels: []GqlSelection, count_aliases_only: bool): usize {
        let total: usize = 0;
        for (sel of sels) {
            switch (sel) {
                .Field f => {
                    if (!count_aliases_only || f.alias != null) {
                        total += 1;
                    }
                    total += _.countSelectionFields(f.selections, count_aliases_only);
                },
                .InlineFragment inf => {
                    total += _.countSelectionFields(inf.selections, count_aliases_only);
                },
                .FragmentSpread => {},
            }
        }
        return total;
    }

    /// Counts the total number of AST field selections across all operations and fragments.
    pub fn totalFields(): usize {
        let total: usize = 0;
        for (op of _.operations) {
            total += _.countSelectionFields(op.selections, false);
        }
        for (frag of _.fragments) {
            total += _.countSelectionFields(frag.selections, false);
        }
        return total;
    }

    /// Counts the total number of aliased field selections across all operations and fragments.
    pub fn totalAliases(): usize {
        let total: usize = 0;
        for (op of _.operations) {
            total += _.countSelectionFields(op.selections, true);
        }
        for (frag of _.fragments) {
            total += _.countSelectionFields(frag.selections, true);
        }
        return total;
    }

    /// Compares two parsed GraphQL documents for full AST structural equality.
    pub fn eql(other: GqlDocument): bool {
        if (_.operations.len != other.operations.len) return false;
        if (_.fragments.len != other.fragments.len) return false;
        let i: usize = 0;
        while (i < _.operations.len) : (i += 1) {
            if (!_.operations[i].eql(other.operations[i])) return false;
        }
        i = 0;
        while (i < _.fragments.len) : (i += 1) {
            if (!_.fragments[i].eql(other.fragments[i])) return false;
        }
        return true;
    }
}

/// GraphQL October 2021 Executable Document recursive-descent parser.
pub struct GqlParser {
    lexer: GqlLexer = GqlLexer.init(""),
    cur: GqlToken = GqlToken{},
    doc_alloc: std.mem.Allocator = undefined,
    opts: GqlParseOptions = GqlParseOptions{},
    node_count: usize = 0,
    field_count: usize = 0,
    alias_count: usize = 0,

    /// Parses a GraphQL executable document with default security guardrails.
    /// Call `doc.deinit()` when finished to release all document memory.
    pub static fn parse(alloc: std.mem.Allocator, src: string): GqlDocument ! GqlError {
        return GqlParser.parseWithOptions(alloc, src, GqlParseOptions{});
    }

    /// Parses a GraphQL executable document with explicit depth/token/node/field/alias guardrails.
    /// Call `doc.deinit()` when finished to release all document memory.
    pub static fn parseWithOptions(alloc: std.mem.Allocator, src: string, opts: GqlParseOptions): GqlDocument ! GqlError {
        let doc_arena = alloc.create(std.heap.ArenaAllocator) catch return GqlError.OutOfMemory;
        unsafe {
            doc_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            doc_arena.deinit();
            alloc.destroy(doc_arena);
        }

        let lex = GqlLexer.initWithLimit(src, opts.max_tokens);
        const first = lex.nextToken()?;

        let parser = GqlParser{
            lexer: lex,
            cur: first,
            doc_alloc: doc_arena.allocator(),
            opts: opts,
            node_count: 0,
            field_count: 0,
            alias_count: 0,
        };

        let doc = parser.parseDocument()?;
        doc.owned_arena = doc_arena;
        doc.parent_alloc = alloc;
        return doc;
    }

    fn bumpNode(*_): void ! GqlError {
        _.node_count += 1;
        if (_.node_count > _.opts.max_nodes) {
            return GqlError.MaxNodesExceeded;
        }
    }

    fn bumpField(*_, has_alias: bool): void ! GqlError {
        _.field_count += 1;
        if (_.field_count > _.opts.max_fields) {
            return GqlError.MaxFieldsExceeded;
        }
        if (has_alias) {
            _.alias_count += 1;
            if (_.alias_count > _.opts.max_aliases) {
                return GqlError.MaxAliasesExceeded;
            }
        }
    }

    fn advance(*_): GqlToken ! GqlError {
        const prev = _.cur;
        _.cur = _.lexer.nextToken()?;
        return prev;
    }

    fn dupeStr(*_, s: string): string ! GqlError {
        return _.doc_alloc.dupe(u8, s) catch return GqlError.OutOfMemory;
    }

    fn expectKind(*_, kind: GqlTokenKind): GqlToken ! GqlError {
        if (_.cur.kind != kind) {
            return GqlError.UnexpectedToken;
        }
        return _.advance();
    }

    fn expectName(*_): string ! GqlError {
        const tok = _.expectKind(.Name)?;
        return _.dupeStr(tok.lexeme);
    }

    fn parseDocument(*_): GqlDocument ! GqlError {
        let ops: std.ArrayList(GqlOperation) = .empty;
        let frags: std.ArrayList(GqlFragmentDef) = .empty;

        while (_.cur.kind != .Eof) {
            if (_.cur.kind == .LBrace) {
                _.bumpNode()?;
                const sels = _.parseSelectionSet(1)?;
                ops.append(_.doc_alloc, GqlOperation{
                    op: .Query,
                    name: null,
                    shorthand: true,
                    var_defs: [],
                    directives: [],
                    selections: sels,
                }) catch return GqlError.OutOfMemory;
            } else if (_.cur.kind == .Name) {
                if (std.mem.eql(u8, _.cur.lexeme, "fragment")) {
                    const frag = _.parseFragmentDef()?;
                    frags.append(_.doc_alloc, frag) catch return GqlError.OutOfMemory;
                } else if (std.mem.eql(u8, _.cur.lexeme, "query") ||
                    std.mem.eql(u8, _.cur.lexeme, "mutation") ||
                    std.mem.eql(u8, _.cur.lexeme, "subscription"))
                {
                    const op = _.parseOperation()?;
                    ops.append(_.doc_alloc, op) catch return GqlError.OutOfMemory;
                } else {
                    return GqlError.UnexpectedToken;
                }
            } else {
                return GqlError.UnexpectedToken;
            }
        }

        if (ops.items.len == 0 && frags.items.len == 0) {
            return GqlError.UnexpectedToken;
        }

        const owned_ops = ops.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
        const owned_frags = frags.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;

        return GqlDocument{
            operations: owned_ops,
            fragments: owned_frags,
            owned_arena: null,
            parent_alloc: null,
        };
    }

    fn parseOperation(*_): GqlOperation ! GqlError {
        _.bumpNode()?;
        const kw = _.expectKind(.Name)?;
        let op_kind: GqlOpKind = .Query;
        if (std.mem.eql(u8, kw.lexeme, "query")) {
            op_kind = .Query;
        } else if (std.mem.eql(u8, kw.lexeme, "mutation")) {
            op_kind = .Mutation;
        } else if (std.mem.eql(u8, kw.lexeme, "subscription")) {
            op_kind = .Subscription;
        } else {
            return GqlError.UnexpectedToken;
        }

        let op_name: string? = null;
        if (_.cur.kind == .Name) {
            op_name = _.expectName()?;
        }

        let var_defs: []GqlVarDef = [];
        if (_.cur.kind == .LParen) {
            var_defs = _.parseVarDefs()?;
        }

        const dirs = _.parseDirectives(false)?;
        const sels = _.parseSelectionSet(1)?;

        return GqlOperation{
            op: op_kind,
            name: op_name,
            shorthand: false,
            var_defs: var_defs,
            directives: dirs,
            selections: sels,
        };
    }

    fn parseFragmentDef(*_): GqlFragmentDef ! GqlError {
        _.bumpNode()?;
        const kw = _.expectKind(.Name)?;
        if (!std.mem.eql(u8, kw.lexeme, "fragment")) {
            return GqlError.UnexpectedToken;
        }

        const frag_name = _.expectName()?;
        if (std.mem.eql(u8, frag_name, "on")) {
            return GqlError.UnexpectedToken;
        }

        const on_tok = _.expectKind(.Name)?;
        if (!std.mem.eql(u8, on_tok.lexeme, "on")) {
            return GqlError.UnexpectedToken;
        }

        const type_cond = _.expectName()?;
        const dirs = _.parseDirectives(false)?;
        const sels = _.parseSelectionSet(1)?;

        return GqlFragmentDef{
            name: frag_name,
            type_condition: type_cond,
            directives: dirs,
            selections: sels,
        };
    }

    fn parseVarDefs(*_): ([]GqlVarDef) ! GqlError {
        * = _.expectKind(.LParen)?;
        let list: std.ArrayList(GqlVarDef) = .empty;

        if (_.cur.kind == .RParen) {
            return GqlError.UnexpectedToken;
        }

        while (_.cur.kind != .RParen) {
            if (_.cur.kind == .Eof) return GqlError.UnexpectedToken;
            _.bumpNode()?;
            * = _.expectKind(.Dollar)?;
            const var_name = _.expectName()?;
            * = _.expectKind(.Colon)?;
            const var_type = _.parseType(1)?;

            let def_val: GqlValue? = null;
            if (_.cur.kind == .Equals) {
                * = _.advance()?;
                def_val = _.parseValue(1, true)?;
            }

            const dirs = _.parseDirectives(true)?;
            list.append(_.doc_alloc, GqlVarDef{
                name: var_name,
                var_type: var_type,
                default_value: def_val,
                directives: dirs,
            }) catch return GqlError.OutOfMemory;
        }

        * = _.expectKind(.RParen)?;
        return list.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
    }

    fn allocTypeBox(*_, inner: GqlType): ([]GqlType) ! GqlError {
        let box = _.doc_alloc.alloc(GqlType, 1) catch return GqlError.OutOfMemory;
        box[0] = inner;
        return box;
    }

    fn parseType(*_, depth: usize): GqlType ! GqlError {
        if (depth > _.opts.max_depth) {
            return GqlError.MaxDepthExceeded;
        }
        _.bumpNode()?;

        let base: GqlType = GqlType.Named("");
        if (_.cur.kind == .LBracket) {
            * = _.advance()?;
            const inner = _.parseType(depth + 1)?;
            * = _.expectKind(.RBracket)?;
            const box = _.allocTypeBox(inner)?;
            base = GqlType.List(box);
        } else if (_.cur.kind == .Name) {
            const n = _.expectName()?;
            base = GqlType.Named(n);
        } else {
            return GqlError.UnexpectedToken;
        }

        if (_.cur.kind == .Bang) {
            * = _.advance()?;
            const box = _.allocTypeBox(base)?;
            return GqlType.NonNull(box);
        }
        return base;
    }

    fn parseDirectives(*_, const_only: bool): ([]GqlDirective) ! GqlError {
        let dirs: std.ArrayList(GqlDirective) = .empty;
        while (_.cur.kind == .At) {
            _.bumpNode()?;
            * = _.advance()?;
            const dir_name = _.expectName()?;
            let args: []GqlArgument = [];
            if (_.cur.kind == .LParen) {
                args = _.parseArguments(const_only)?;
            }
            dirs.append(_.doc_alloc, GqlDirective{
                name: dir_name,
                args: args,
            }) catch return GqlError.OutOfMemory;
        }
        return dirs.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
    }

    fn parseArguments(*_, const_only: bool): ([]GqlArgument) ! GqlError {
        * = _.expectKind(.LParen)?;
        if (_.cur.kind == .RParen) {
            return GqlError.UnexpectedToken;
        }
        let args: std.ArrayList(GqlArgument) = .empty;
        while (_.cur.kind != .RParen) {
            if (_.cur.kind == .Eof) return GqlError.UnexpectedToken;
            _.bumpNode()?;
            const arg_name = _.expectName()?;
            * = _.expectKind(.Colon)?;
            const arg_val = _.parseValue(1, const_only)?;
            args.append(_.doc_alloc, GqlArgument{
                name: arg_name,
                value: arg_val,
            }) catch return GqlError.OutOfMemory;
        }
        * = _.expectKind(.RParen)?;
        return args.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
    }

    fn parseSelectionSet(*_, depth: usize): ([]GqlSelection) ! GqlError {
        if (depth > _.opts.max_depth) {
            return GqlError.MaxDepthExceeded;
        }
        * = _.expectKind(.LBrace)?;
        if (_.cur.kind == .RBrace) {
            return GqlError.UnexpectedToken;
        }

        let sels: std.ArrayList(GqlSelection) = .empty;
        while (_.cur.kind != .RBrace) {
            if (_.cur.kind == .Eof) return GqlError.UnexpectedToken;

            if (_.cur.kind == .Spread) {
                _.bumpNode()?;
                * = _.advance()?;
                if (_.cur.kind == .Name && std.mem.eql(u8, _.cur.lexeme, "on")) {
                    * = _.advance()?;
                    const type_cond = _.expectName()?;
                    const dirs = _.parseDirectives(false)?;
                    const sub_sels = _.parseSelectionSet(depth + 1)?;
                    sels.append(_.doc_alloc, GqlSelection.InlineFragment(GqlInlineFragment{
                        type_condition: type_cond,
                        directives: dirs,
                        selections: sub_sels,
                    })) catch return GqlError.OutOfMemory;
                } else if (_.cur.kind == .At || _.cur.kind == .LBrace) {
                    const dirs = _.parseDirectives(false)?;
                    const sub_sels = _.parseSelectionSet(depth + 1)?;
                    sels.append(_.doc_alloc, GqlSelection.InlineFragment(GqlInlineFragment{
                        type_condition: null,
                        directives: dirs,
                        selections: sub_sels,
                    })) catch return GqlError.OutOfMemory;
                } else if (_.cur.kind == .Name) {
                    const frag_name = _.expectName()?;
                    const dirs = _.parseDirectives(false)?;
                    sels.append(_.doc_alloc, GqlSelection.FragmentSpread(GqlFragmentSpread{
                        name: frag_name,
                        directives: dirs,
                    })) catch return GqlError.OutOfMemory;
                } else {
                    return GqlError.UnexpectedToken;
                }
            } else if (_.cur.kind == .Name) {
                _.bumpNode()?;
                const first_name = _.expectName()?;
                let alias_name: string? = null;
                let field_name = first_name;

                if (_.cur.kind == .Colon) {
                    * = _.advance()?;
                    alias_name = first_name;
                    field_name = _.expectName()?;
                }

                _.bumpField(alias_name != null)?;

                let args: []GqlArgument = [];
                if (_.cur.kind == .LParen) {
                    args = _.parseArguments(false)?;
                }

                const dirs = _.parseDirectives(false)?;

                let sub_sels: []GqlSelection = [];
                if (_.cur.kind == .LBrace) {
                    sub_sels = _.parseSelectionSet(depth + 1)?;
                }

                sels.append(_.doc_alloc, GqlSelection.Field(GqlField{
                    alias: alias_name,
                    name: field_name,
                    args: args,
                    directives: dirs,
                    selections: sub_sels,
                })) catch return GqlError.OutOfMemory;
            } else {
                return GqlError.UnexpectedToken;
            }
        }

        * = _.expectKind(.RBrace)?;
        return sels.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
    }

    fn parseValue(*_, depth: usize, const_only: bool): GqlValue ! GqlError {
        if (depth > _.opts.max_depth) {
            return GqlError.MaxDepthExceeded;
        }
        _.bumpNode()?;

        switch (_.cur.kind) {
            .Dollar => {
                if (const_only) {
                    return GqlError.UnexpectedToken;
                }
                * = _.advance()?;
                const var_name = _.expectName()?;
                return GqlValue.Variable(var_name);
            },
            .Int => {
                const tok = _.advance()?;
                const parsed = std.fmt.parseInt(i64, tok.lexeme, 10) catch return GqlError.InvalidNumber;
                return GqlValue.Int(parsed);
            },
            .Float => {
                const tok = _.advance()?;
                const parsed = std.fmt.parseFloat(f64, tok.lexeme) catch return GqlError.InvalidNumber;
                if (std.math.isNan(parsed) || std.math.isInf(parsed)) {
                    return GqlError.InvalidNumber;
                }
                return GqlValue.Float(parsed);
            },
            .String => {
                const tok = _.advance()?;
                const decoded = GqlLexer.decodeString(_.doc_alloc, tok.lexeme)?;
                return GqlValue.String(decoded);
            },
            .Name => {
                const tok = _.advance()?;
                if (std.mem.eql(u8, tok.lexeme, "true")) {
                    return GqlValue.Bool(true);
                }
                if (std.mem.eql(u8, tok.lexeme, "false")) {
                    return GqlValue.Bool(false);
                }
                if (std.mem.eql(u8, tok.lexeme, "null")) {
                    return GqlValue.Null;
                }
                const owned = _.dupeStr(tok.lexeme)?;
                return GqlValue.Enum(owned);
            },
            .LBracket => {
                * = _.advance()?;
                let items: std.ArrayList(GqlValue) = .empty;
                while (_.cur.kind != .RBracket) {
                    if (_.cur.kind == .Eof) return GqlError.UnexpectedToken;
                    const elem = _.parseValue(depth + 1, const_only)?;
                    items.append(_.doc_alloc, elem) catch return GqlError.OutOfMemory;
                }
                * = _.expectKind(.RBracket)?;
                const owned_items = items.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
                return GqlValue.List(owned_items);
            },
            .LBrace => {
                * = _.advance()?;
                let fields: std.ArrayList(GqlObjectField) = .empty;
                while (_.cur.kind != .RBrace) {
                    if (_.cur.kind == .Eof) return GqlError.UnexpectedToken;
                    _.bumpNode()?;
                    const fname = _.expectName()?;
                    * = _.expectKind(.Colon)?;
                    const fval = _.parseValue(depth + 1, const_only)?;
                    fields.append(_.doc_alloc, GqlObjectField{
                        name: fname,
                        value: fval,
                    }) catch return GqlError.OutOfMemory;
                }
                * = _.expectKind(.RBrace)?;
                const owned_fields = fields.toOwnedSlice(_.doc_alloc) catch return GqlError.OutOfMemory;
                return GqlValue.Object(owned_fields);
            },
            else => return GqlError.UnexpectedToken,
        }
    }
}
