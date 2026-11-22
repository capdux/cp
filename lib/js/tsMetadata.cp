// Source: Absorbed from /data/dev/bun/src/js_parser/ast/TypeScript.cp | Synced: 2026-11-22T19:09:44Z
// ============================================================================
// lib/js/tsMetadata.cp — Zero-Alloc TypeScript Decorator Metadata Serializer
// & Type-Argument Lookahead Classifier
// ============================================================================
// Implements Bun's `js_parser/ast/TypeScript.cp` (`Metadata` union/intersection
// algebra for `emitDecoratorMetadata` and `canFollowTypeArgumentsInExpression`
// disambiguation) in 100% pure CP with zero dynamic heap allocation.

pub err TsMetadataError {
    BufferTooSmall,
}

/// TypeScript `emitDecoratorMetadata` type node kind (1:1 port of Bun's `TypeScript.Metadata`).
pub enum TsMetadataKind: u8 {
    m_none = 0,
    m_never = 1,
    m_unknown = 2,
    m_any = 3,
    m_void = 4,
    m_null = 5,
    m_undefined = 6,
    m_function = 7,
    m_array = 8,
    m_boolean = 9,
    m_string = 10,
    m_object = 11,
    m_number = 12,
    m_bigint = 13,
    m_symbol = 14,
    m_promise = 15,
    m_identifier = 16,
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

/// Zero-allocation TypeScript decorator metadata value supporting union/intersection
/// reduction and runtime constructor serialization (`String`, `Number`, `Object`, `void 0`, etc.).
pub struct TsDecoratorMetadata {
    kind: TsMetadataKind = TsMetadataKind.m_none,
    ident_name: []const u8 = "",

    static fn fromKind(k: TsMetadataKind): TsDecoratorMetadata {
        return TsDecoratorMetadata{ kind: k, ident_name: "" };
    }

    static fn identifier(name: []const u8): TsDecoratorMetadata {
        return TsDecoratorMetadata{
            kind: TsMetadataKind.m_identifier,
            ident_name: name,
        };
    }

    fn eql(other: TsDecoratorMetadata): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == TsMetadataKind.m_identifier) {
            return strEq(_.ident_name, other.ident_name);
        }
        return true;
    }

    /// Returns the final union type if merging can short-circuit, or `null` to continue.
    /// Converts `m_never`/`m_null`/`m_undefined` to `m_none` so the next branch replaces it
    /// (aligned with Bun's `Metadata.finishUnion`).
    fn finishUnion(*_): TsDecoratorMetadata? {
        if (_.kind == TsMetadataKind.m_identifier) {
            if (strEq(_.ident_name, "Object")) {
                return TsDecoratorMetadata.fromKind(TsMetadataKind.m_object);
            }
            return null;
        }
        if (_.kind == TsMetadataKind.m_unknown ||
            _.kind == TsMetadataKind.m_any ||
            _.kind == TsMetadataKind.m_object)
        {
            return TsDecoratorMetadata.fromKind(TsMetadataKind.m_object);
        }
        if (_.kind == TsMetadataKind.m_never ||
            _.kind == TsMetadataKind.m_null ||
            _.kind == TsMetadataKind.m_undefined)
        {
            _.kind = TsMetadataKind.m_none;
            _.ident_name = "";
            return null;
        }
        return null;
    }

    /// Merges `left` into this right-hand union branch (aligned with Bun's `Metadata.mergeUnion`).
    fn mergeUnion(*_, left: TsDecoratorMetadata) {
        if (left.kind == TsMetadataKind.m_none) return;
        if (_.kind != left.kind) {
            if (_.kind == TsMetadataKind.m_never ||
                _.kind == TsMetadataKind.m_undefined ||
                _.kind == TsMetadataKind.m_null)
            {
                _.kind = left.kind;
                _.ident_name = left.ident_name;
            } else {
                _.kind = TsMetadataKind.m_object;
                _.ident_name = "";
            }
        } else if (_.kind == TsMetadataKind.m_identifier) {
            if (!strEq(_.ident_name, left.ident_name)) {
                _.kind = TsMetadataKind.m_object;
                _.ident_name = "";
            }
        }
    }

    /// Returns the final intersection type if merging can short-circuit, or `null` to continue.
    /// Ensures `m_never` dominates and `m_unknown`/`m_null`/`m_undefined` become `m_none`
    /// (aligned with Bun's `Metadata.finishIntersection`).
    fn finishIntersection(*_): TsDecoratorMetadata? {
        if (_.kind == TsMetadataKind.m_identifier) {
            if (strEq(_.ident_name, "Object")) {
                return TsDecoratorMetadata.fromKind(TsMetadataKind.m_object);
            }
            return null;
        }
        if (_.kind == TsMetadataKind.m_never) {
            return TsDecoratorMetadata.fromKind(TsMetadataKind.m_never);
        }
        if (_.kind == TsMetadataKind.m_any || _.kind == TsMetadataKind.m_object) {
            return TsDecoratorMetadata.fromKind(TsMetadataKind.m_object);
        }
        if (_.kind == TsMetadataKind.m_unknown ||
            _.kind == TsMetadataKind.m_null ||
            _.kind == TsMetadataKind.m_undefined)
        {
            _.kind = TsMetadataKind.m_none;
            _.ident_name = "";
            return null;
        }
        return null;
    }

    /// Merges `left` into this right-hand intersection branch (aligned with Bun's `Metadata.mergeIntersection`).
    fn mergeIntersection(*_, left: TsDecoratorMetadata) {
        if (left.kind != TsMetadataKind.m_none) {
            if (_.kind != left.kind) {
                if (_.kind == TsMetadataKind.m_unknown ||
                    _.kind == TsMetadataKind.m_undefined ||
                    _.kind == TsMetadataKind.m_null)
                {
                    _.kind = left.kind;
                    _.ident_name = left.ident_name;
                } else if (_.kind == TsMetadataKind.m_never) {
                    // m_never dominates intersection
                } else {
                    _.kind = TsMetadataKind.m_object;
                    _.ident_name = "";
                }
            } else if (_.kind == TsMetadataKind.m_identifier) {
                if (!strEq(_.ident_name, left.ident_name)) {
                    _.kind = TsMetadataKind.m_object;
                    _.ident_name = "";
                }
            }
        } else {
            // Intersection of only `unknown` serializes to `undefined` (`void 0`), not `Object`
            if (_.kind == TsMetadataKind.m_unknown) {
                _.kind = TsMetadataKind.m_undefined;
                _.ident_name = "";
            }
        }
    }

    /// Reduces a slice of union branches (`T1 | T2 | ...`) to a single decorator metadata type.
    static fn foldUnion(items: []const TsDecoratorMetadata): TsDecoratorMetadata {
        let acc = TsDecoratorMetadata.fromKind(TsMetadataKind.m_none);
        let i: usize = 0;
        while (i < items.len) {
            let cur = items[i];
            if (done := cur.finishUnion()) {
                return done;
            }
            cur.mergeUnion(acc);
            acc = cur;
            i += 1;
        }
        if (acc.kind == TsMetadataKind.m_none) {
            return TsDecoratorMetadata.fromKind(TsMetadataKind.m_undefined);
        }
        return acc;
    }

    /// Reduces a slice of intersection branches (`T1 & T2 & ...`) to a single decorator metadata type.
    static fn foldIntersection(items: []const TsDecoratorMetadata): TsDecoratorMetadata {
        let acc = TsDecoratorMetadata.fromKind(TsMetadataKind.m_none);
        let i: usize = 0;
        while (i < items.len) {
            let cur = items[i];
            if (done := cur.finishIntersection()) {
                return done;
            }
            cur.mergeIntersection(acc);
            acc = cur;
            i += 1;
        }
        if (acc.kind == TsMetadataKind.m_none) {
            return TsDecoratorMetadata.fromKind(TsMetadataKind.m_undefined);
        }
        return acc;
    }

    /// Returns the runtime JS constructor/expression string for `Reflect.metadata`.
    fn runtimeRefStr(): []const u8 {
        return switch (_.kind) {
            .m_none, .m_void, .m_null, .m_undefined, .m_never => "void 0",
            .m_unknown, .m_any, .m_object => "Object",
            .m_function => "Function",
            .m_array => "Array",
            .m_boolean => "Boolean",
            .m_string => "String",
            .m_number => "Number",
            .m_bigint => "BigInt",
            .m_symbol => "Symbol",
            .m_promise => "Promise",
            .m_identifier => _.ident_name,
        };
    }

    fn emitRuntimeRef(out: []u8): TsMetadataError!usize {
        const s = _.runtimeRefStr();
        if (s.len > out.len) return TsMetadataError.BufferTooSmall;
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i += 1;
        }
        return s.len;
    }
}

/// Token class immediately following `>` used to disambiguate `foo<T>(x)` vs `a < b > c`
/// (aligned with Bun's `canFollowTypeArgumentsInExpression`).
pub enum TsTypeArgLookaheadToken: u8 {
    open_paren = 0,
    no_sub_template = 1,
    template_head = 2,
    less_than = 3,
    greater_than = 4,
    plus = 5,
    minus = 6,
    greater_than_eq = 7,
    shift_right = 8,
    binary_op = 9,
    expr_start = 10,
    non_expr_punct = 11,
}

/// Decides whether `tok` may legally follow a generic type argument list `foo<T>` in an expression.
pub fn canFollowTsTypeArguments(tok: TsTypeArgLookaheadToken, has_newline_before: bool): bool {
    return switch (tok) {
        .open_paren, .no_sub_template, .template_head => true,
        .less_than, .greater_than, .plus, .minus, .greater_than_eq, .shift_right => false,
        .binary_op, .non_expr_punct => true,
        .expr_start => has_newline_before,
    };
}
