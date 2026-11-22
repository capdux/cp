// Source: Absorbed from /data/dev/bun/src/js_parser/ast/G.cp | Synced: 2026-11-22T10:07:22Z
// ============================================================================
// lib/js/classAst.cp — Zero-Alloc JS/TS Class, Property & Function AST
// Descriptors, Tree-Shaking `canBeMoved` Analysis & Arity/Lowering Queries
// ============================================================================
// Absorbed from Bun `js_parser/ast/G.cp`. Uses caller-provided external slices
// (`[]const JsClassMember`, `[]const JsFnParamDesc`) with zero heap allocation
// and no hardcoded small limits.

pub const JS_MEMBER_FLAG_NONE: u16 = 0x0000;
pub const JS_MEMBER_FLAG_STATIC: u16 = 0x0001;
pub const JS_MEMBER_FLAG_COMPUTED: u16 = 0x0002;
pub const JS_MEMBER_FLAG_SPREAD: u16 = 0x0004;
pub const JS_MEMBER_FLAG_METHOD: u16 = 0x0008;
pub const JS_MEMBER_FLAG_PRIVATE: u16 = 0x0010;
pub const JS_MEMBER_FLAG_DECORATED: u16 = 0x0020;
pub const JS_MEMBER_FLAG_READONLY: u16 = 0x0040;
pub const JS_MEMBER_FLAG_OVERRIDE: u16 = 0x0080;

pub const JS_FN_FLAG_NONE: u16 = 0x0000;
pub const JS_FN_FLAG_ASYNC: u16 = 0x0001;
pub const JS_FN_FLAG_GENERATOR: u16 = 0x0002;
pub const JS_FN_FLAG_ARROW: u16 = 0x0004;
pub const JS_FN_FLAG_USES_ARGUMENTS: u16 = 0x0008;
pub const JS_FN_FLAG_STRICT: u16 = 0x0010;

pub err JsClassAstError {
    BufferTooSmall,
    InvalidRestParam,
}

/// Property/member kind matching Bun `G.PropertyKind`.
pub enum JsPropertyKind: u8 {
    normal = 0,
    get = 1,
    set = 2,
    spread = 3,
    declare_prop = 4,
    abstract_prop = 5,
    class_static_block = 6,
    auto_accessor = 7,

    fn asStr(): []const u8 {
        if (_ == JsPropertyKind.normal) return "normal";
        if (_ == JsPropertyKind.get) return "get";
        if (_ == JsPropertyKind.set) return "set";
        if (_ == JsPropertyKind.spread) return "spread";
        if (_ == JsPropertyKind.declare_prop) return "declare";
        if (_ == JsPropertyKind.abstract_prop) return "abstract";
        if (_ == JsPropertyKind.class_static_block) return "class_static_block";
        return "auto_accessor";
    }
}

/// Purity classification of a class property's value or initializer expression.
pub enum JsExprPurity: u8 {
    none = 0,
    function_or_arrow = 1,
    pure_movable = 2,
    side_effectful = 3,

    fn canMoveInStaticField(): bool {
        return _ != JsExprPurity.side_effectful;
    }
}

/// Diagnostic reason why a class declaration/expression cannot be moved or tree-shaken.
pub enum JsClassMoveBlocker: u8 {
    none = 0,
    has_extends = 1,
    has_decorators = 2,
    has_static_block = 3,
    has_computed_key = 4,
    has_spread = 5,
    impure_static_field = 6,

    fn asStr(): []const u8 {
        if (_ == JsClassMoveBlocker.none) return "none";
        if (_ == JsClassMoveBlocker.has_extends) return "has_extends";
        if (_ == JsClassMoveBlocker.has_decorators) return "has_decorators";
        if (_ == JsClassMoveBlocker.has_static_block) return "has_static_block";
        if (_ == JsClassMoveBlocker.has_computed_key) return "has_computed_key";
        if (_ == JsClassMoveBlocker.has_spread) return "has_spread";
        return "impure_static_field";
    }
}

/// Descriptor for a single class member (`G.Property`).
pub struct JsClassMember {
    name: []const u8 = "",
    kind: JsPropertyKind = JsPropertyKind.normal,
    flags: u16 = JS_MEMBER_FLAG_NONE,
    value_purity: JsExprPurity = JsExprPurity.none,
    init_purity: JsExprPurity = JsExprPurity.none,

    static fn method(name: []const u8, is_static: bool): JsClassMember {
        return JsClassMember{
            name: name,
            kind: JsPropertyKind.normal,
            flags: JS_MEMBER_FLAG_METHOD | (if (is_static) JS_MEMBER_FLAG_STATIC else 0),
            value_purity: JsExprPurity.function_or_arrow,
            init_purity: JsExprPurity.none,
        };
    }

    static fn field(name: []const u8, is_static: bool, init_purity: JsExprPurity): JsClassMember {
        return JsClassMember{
            name: name,
            kind: JsPropertyKind.normal,
            flags: if (is_static) JS_MEMBER_FLAG_STATIC else JS_MEMBER_FLAG_NONE,
            value_purity: JsExprPurity.none,
            init_purity: init_purity,
        };
    }

    static fn staticBlock(): JsClassMember {
        return JsClassMember{
            name: "",
            kind: JsPropertyKind.class_static_block,
            flags: JS_MEMBER_FLAG_STATIC,
            value_purity: JsExprPurity.side_effectful,
            init_purity: JsExprPurity.none,
        };
    }

    static fn autoAccessor(name: []const u8, is_static: bool, init_purity: JsExprPurity): JsClassMember {
        return JsClassMember{
            name: name,
            kind: JsPropertyKind.auto_accessor,
            flags: if (is_static) JS_MEMBER_FLAG_STATIC else JS_MEMBER_FLAG_NONE,
            value_purity: JsExprPurity.none,
            init_purity: init_purity,
        };
    }

    fn hasFlag(mask: u16): bool {
        return (_.flags & mask) != 0;
    }

    fn isStatic(): bool {
        return _.hasFlag(JS_MEMBER_FLAG_STATIC);
    }

    fn isComputed(): bool {
        return _.hasFlag(JS_MEMBER_FLAG_COMPUTED);
    }

    fn isSpread(): bool {
        return _.hasFlag(JS_MEMBER_FLAG_SPREAD) || _.kind == JsPropertyKind.spread;
    }

    fn isPrivate(): bool {
        return _.hasFlag(JS_MEMBER_FLAG_PRIVATE);
    }

    fn isDecorated(): bool {
        return _.hasFlag(JS_MEMBER_FLAG_DECORATED);
    }
}

/// External-slice-backed descriptor for a JS/TS class (`G.Class`).
pub struct JsClassDesc {
    name: []const u8 = "",
    has_extends: bool = false,
    has_decorators: bool = false,
    should_lower_standard_decorators: bool = false,
    members: []const JsClassMember,

    /// Exact Bun `G.Class.canBeMoved()` semantics with structured blocker diagnostics.
    fn moveBlocker(): JsClassMoveBlocker {
        if (_.has_extends) return JsClassMoveBlocker.has_extends;
        if (_.has_decorators) return JsClassMoveBlocker.has_decorators;

        let i: usize = 0;
        while (i < _.members.len) {
            const m = _.members[i];
            if (m.kind == JsPropertyKind.class_static_block) {
                return JsClassMoveBlocker.has_static_block;
            }
            if (m.isDecorated()) {
                return JsClassMoveBlocker.has_decorators;
            }
            if (m.isComputed()) {
                return JsClassMoveBlocker.has_computed_key;
            }
            if (m.isSpread()) {
                return JsClassMoveBlocker.has_spread;
            }
            if (m.kind == JsPropertyKind.normal && m.isStatic()) {
                if (!m.value_purity.canMoveInStaticField() || !m.init_purity.canMoveInStaticField()) {
                    return JsClassMoveBlocker.impure_static_field;
                }
            }
            i += 1;
        }
        return JsClassMoveBlocker.none;
    }

    /// True if the class declaration/expression has no evaluation side effects and can be moved/reordered.
    fn canBeMoved(): bool {
        return _.moveBlocker() == JsClassMoveBlocker.none;
    }

    fn hasStaticBlock(): bool {
        let i: usize = 0;
        while (i < _.members.len) {
            if (_.members[i].kind == JsPropertyKind.class_static_block) return true;
            i += 1;
        }
        return false;
    }

    fn hasPrivateMembers(): bool {
        let i: usize = 0;
        while (i < _.members.len) {
            if (_.members[i].isPrivate()) return true;
            i += 1;
        }
        return false;
    }

    fn hasAutoAccessors(): bool {
        let i: usize = 0;
        while (i < _.members.len) {
            if (_.members[i].kind == JsPropertyKind.auto_accessor) return true;
            i += 1;
        }
        return false;
    }

    fn countStaticFields(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.members.len) {
            const m = _.members[i];
            if (m.kind == JsPropertyKind.normal && m.isStatic() && !m.hasFlag(JS_MEMBER_FLAG_METHOD)) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    fn countInstanceMembers(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.members.len) {
            const m = _.members[i];
            if (!m.isStatic() && m.kind != JsPropertyKind.class_static_block) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// True if this class requires syntax lowering for the given ECMAScript target year (e.g. 2020, 2022).
    fn needsLowering(es_target_year: u16): bool {
        if (_.should_lower_standard_decorators || _.has_decorators || _.hasAutoAccessors()) {
            return true;
        }
        if (es_target_year < 2022) {
            if (_.hasStaticBlock() || _.hasPrivateMembers() || _.countStaticFields() > 0) {
                return true;
            }
        }
        if (es_target_year < 2015) {
            return true;
        }
        return false;
    }
}

/// Function parameter descriptor matching Bun `G.Arg`.
pub struct JsFnParamDesc {
    name: []const u8 = "",
    has_default: bool = false,
    is_rest: bool = false,
    is_ts_ctor_field: bool = false,
    has_decorators: bool = false,

    static fn simple(name: []const u8): JsFnParamDesc {
        return JsFnParamDesc{ name: name };
    }

    static fn withDefault(name: []const u8): JsFnParamDesc {
        return JsFnParamDesc{ name: name, has_default: true };
    }

    static fn rest(name: []const u8): JsFnParamDesc {
        return JsFnParamDesc{ name: name, is_rest: true };
    }

    static fn tsCtorField(name: []const u8, has_default: bool): JsFnParamDesc {
        return JsFnParamDesc{
            name: name,
            has_default: has_default,
            is_ts_ctor_field: true,
        };
    }
}

fn appendByte(out: []u8, pos: usize, ch: u8): JsClassAstError!usize {
    if (pos >= out.len) return JsClassAstError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): JsClassAstError!usize {
    if (src.len > out.len - pos) return JsClassAstError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// External-slice-backed JS/TS function signature descriptor (`G.Fn`).
pub struct JsFnSignature {
    name: []const u8 = "",
    flags: u16 = JS_FN_FLAG_NONE,
    params: []const JsFnParamDesc,

    fn isAsync(): bool {
        return (_.flags & JS_FN_FLAG_ASYNC) != 0;
    }

    fn isGenerator(): bool {
        return (_.flags & JS_FN_FLAG_GENERATOR) != 0;
    }

    fn isArrow(): bool {
        return (_.flags & JS_FN_FLAG_ARROW) != 0;
    }

    fn usesArguments(): bool {
        return (_.flags & JS_FN_FLAG_USES_ARGUMENTS) != 0;
    }

    /// Validates ECMAScript parameter list rules (rest parameter must be last and have no default).
    fn validate(): JsClassAstError!void {
        let i: usize = 0;
        while (i < _.params.len) {
            const p = _.params[i];
            if (p.is_rest) {
                if (i + 1 != _.params.len || p.has_default) {
                    return JsClassAstError.InvalidRestParam;
                }
            }
            i += 1;
        }
    }

    /// Computes ECMAScript `Function.prototype.length` (parameters before first default or rest).
    fn minArity(): usize {
        let i: usize = 0;
        while (i < _.params.len) {
            if (_.params[i].has_default || _.params[i].is_rest) {
                return i;
            }
            i += 1;
        }
        return _.params.len;
    }

    /// Returns maximum fixed parameter count, or `null` if the function has a rest parameter.
    fn maxArity(): usize? {
        if (_.params.len > 0 && _.params[_.params.len - 1].is_rest) {
            return null;
        }
        return _.params.len;
    }

    fn countTsCtorFields(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.params.len) {
            if (_.params[i].is_ts_ctor_field) count += 1;
            i += 1;
        }
        return count;
    }

    /// Formats a compact function header (e.g., `async function* fetch(url, ...opts)` or `async (a, b) =>`).
    fn formatHeader(out: []u8): JsClassAstError!usize {
        _.validate()?;
        let pos: usize = 0;
        if (_.isAsync()) {
            pos = appendSlice(out, pos, "async ")?;
        }
        if (!_.isArrow()) {
            pos = appendSlice(out, pos, "function")?;
            if (_.isGenerator()) {
                pos = appendByte(out, pos, '*')?;
            }
            if (_.name.len > 0) {
                pos = appendByte(out, pos, ' ')?;
                pos = appendSlice(out, pos, _.name)?;
            }
        }
        pos = appendByte(out, pos, '(')?;
        let i: usize = 0;
        while (i < _.params.len) {
            if (i > 0) {
                pos = appendSlice(out, pos, ", ")?;
            }
            if (_.params[i].is_rest) {
                pos = appendSlice(out, pos, "...")?;
            }
            pos = appendSlice(out, pos, _.params[i].name)?;
            i += 1;
        }
        pos = appendByte(out, pos, ')')?;
        if (_.isArrow()) {
            pos = appendSlice(out, pos, " =>")?;
        }
        return pos;
    }
}
