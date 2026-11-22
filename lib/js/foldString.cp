// Source: Absorbed from /data/dev/bun/src/js_parser/ast/foldStringAddition.cp | Synced: 2026-11-22T16:18:43Z
// Pure-CP zero-allocation JS/TS compile-time string & template literal addition folder.
// Aligns with Bun's AST constant folding semantics (Normal vs NestedLeft, tagged template guard,
// side-effect-free primitive coercion, and empty-string identity elimination).

pub const FOLD_MAX_STR_LEN: usize = 512;
pub const FOLD_MAX_PART_TAIL_LEN: usize = 128;
pub const FOLD_MAX_TEMPLATE_PARTS: usize = 16;

/// Controls whether coercing the left operand to a string is safe.
/// - `Normal`: `"x" + "y" -> "xy"`, `1 + "y" -> "1y"`
/// - `NestedLeft`: inside `(a + 1) + "y"`, `1 + "y"` MUST NOT be folded because `a + 1` may be numeric;
///   however `(a + "x") + "y" -> a + "xy"` remains safe.
pub enum FoldStringKind {
    Normal,
    NestedLeft,
}

pub enum JsPrimitiveType {
    Unknown,
    String,
    Number,
    Boolean,
    Null,
    Undefined,
}

pub enum JsFoldExprKind {
    StringLit,
    InlinedEnumString,
    NumberInt,
    BoolLit,
    NullLit,
    UndefinedLit,
    TemplateLit,
    DynamicExpr,
}

pub struct JsTemplatePart {
    expr_id: u32 = 0,
    tail_buf: [FOLD_MAX_PART_TAIL_LEN]u8 = [0; FOLD_MAX_PART_TAIL_LEN],
    tail_len: usize = 0,

    static fn init(expr_id: u32, tail: []const u8): JsTemplatePart? {
        if (tail.len > FOLD_MAX_PART_TAIL_LEN) return null;
        let part = JsTemplatePart { expr_id: expr_id };
        let i: usize = 0;
        while (i < tail.len) {
            part.tail_buf[i] = tail[i];
            i += 1;
        }
        part.tail_len = tail.len;
        return part;
    }

    fn tailSlice(): []const u8 {
        return _.tail_buf[0.._.tail_len];
    }

    fn appendTail(*_, extra: []const u8): bool {
        if (_.tail_len + extra.len > FOLD_MAX_PART_TAIL_LEN) return false;
        let i: usize = 0;
        while (i < extra.len) {
            _.tail_buf[_.tail_len + i] = extra[i];
            i += 1;
        }
        _.tail_len += extra.len;
        return true;
    }
}

/// Zero-allocation decimal formatter for signed 64-bit integers.
pub fn formatJsInt(val: i64, out: []u8): ([]const u8)? {
    let tmp: [24]u8 = [0; 24];
    let pos: usize = 0;
    let is_neg = false;
    let mag: u64 = 0;

    if (val < 0) {
        is_neg = true;
        // Avoid i64 negation overflow for MIN_I64 (-9223372036854775808)
        mag = @as(u64, -(val + 1)) + 1;
    } else {
        mag = @as(u64, val);
    }

    if (mag == 0) {
        tmp[pos] = '0';
        pos += 1;
    } else {
        while (mag > 0) {
            tmp[pos] = @as(u8, '0') + @as(u8, mag % 10);
            pos += 1;
            mag /= 10;
        }
    }

    const total = pos + (if (is_neg) @as(usize, 1) else @as(usize, 0));
    if (total > out.len) return null;

    let out_pos: usize = 0;
    if (is_neg) {
        out[out_pos] = '-';
        out_pos += 1;
    }
    let k: usize = pos;
    while (k > 0) {
        k -= 1;
        out[out_pos] = tmp[k];
        out_pos += 1;
    }
    return out[0..out_pos];
}

fn copyIntoBuf(dst: []u8, src: []const u8): usize? {
    if (src.len > dst.len) return null;
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
    return src.len;
}

fn concatIntoBuf(dst: []u8, a: []const u8, b: []const u8): usize? {
    if (a.len + b.len > dst.len) return null;
    let i: usize = 0;
    while (i < a.len) {
        dst[i] = a[i];
        i += 1;
    }
    let j: usize = 0;
    while (j < b.len) {
        dst[a.len + j] = b[j];
        j += 1;
    }
    return a.len + b.len;
}

/// Zero-allocation JS expression node for compile-time string/template folding.
pub struct JsFoldExpr {
    kind: JsFoldExprKind = JsFoldExprKind.UndefinedLit,
    str_buf: [FOLD_MAX_STR_LEN]u8 = [0; FOLD_MAX_STR_LEN],
    str_len: usize = 0,
    int_val: i64 = 0,
    bool_val: bool = false,
    expr_id: u32 = 0,
    known_prim: JsPrimitiveType = JsPrimitiveType.Unknown,
    is_tagged: bool = false,
    prefer_template: bool = false,
    parts: [FOLD_MAX_TEMPLATE_PARTS]JsTemplatePart = [JsTemplatePart {}; FOLD_MAX_TEMPLATE_PARTS],
    parts_len: usize = 0,

    static fn stringLit(s: []const u8): JsFoldExpr? {
        let e = JsFoldExpr {
            kind: JsFoldExprKind.StringLit,
            known_prim: JsPrimitiveType.String,
        };
        const n = copyIntoBuf(e.str_buf[0..], s) ?? return null;
        e.str_len = n;
        return e;
    }

    static fn inlinedEnumString(s: []const u8): JsFoldExpr? {
        let e = JsFoldExpr {
            kind: JsFoldExprKind.InlinedEnumString,
            known_prim: JsPrimitiveType.String,
        };
        const n = copyIntoBuf(e.str_buf[0..], s) ?? return null;
        e.str_len = n;
        return e;
    }

    static fn numberInt(v: i64): JsFoldExpr {
        return JsFoldExpr {
            kind: JsFoldExprKind.NumberInt,
            int_val: v,
            known_prim: JsPrimitiveType.Number,
        };
    }

    static fn boolLit(v: bool): JsFoldExpr {
        return JsFoldExpr {
            kind: JsFoldExprKind.BoolLit,
            bool_val: v,
            known_prim: JsPrimitiveType.Boolean,
        };
    }

    static fn nullLit(): JsFoldExpr {
        return JsFoldExpr {
            kind: JsFoldExprKind.NullLit,
            known_prim: JsPrimitiveType.Null,
        };
    }

    static fn undefinedLit(): JsFoldExpr {
        return JsFoldExpr {
            kind: JsFoldExprKind.UndefinedLit,
            known_prim: JsPrimitiveType.Undefined,
        };
    }

    static fn templateLit(head: []const u8, tagged: bool): JsFoldExpr? {
        let e = JsFoldExpr {
            kind: JsFoldExprKind.TemplateLit,
            known_prim: if (tagged) JsPrimitiveType.Unknown else JsPrimitiveType.String,
            is_tagged: tagged,
            prefer_template: true,
        };
        const n = copyIntoBuf(e.str_buf[0..], head) ?? return null;
        e.str_len = n;
        return e;
    }

    static fn dynamicExpr(expr_id: u32, known_prim: JsPrimitiveType): JsFoldExpr {
        return JsFoldExpr {
            kind: JsFoldExprKind.DynamicExpr,
            expr_id: expr_id,
            known_prim: known_prim,
        };
    }

    fn addTemplatePart(*_, expr_id: u32, tail: []const u8): bool {
        if (_.kind != JsFoldExprKind.TemplateLit) return false;
        if (_.parts_len >= FOLD_MAX_TEMPLATE_PARTS) return false;
        const part = JsTemplatePart.init(expr_id, tail) ?? return false;
        _.parts[_.parts_len] = part;
        _.parts_len += 1;
        return true;
    }

    fn strSlice(): []const u8 {
        return _.str_buf[0.._.str_len];
    }

    fn templateHead(): []const u8 {
        return _.str_buf[0.._.str_len];
    }

    fn templatePartCount(): usize {
        return _.parts_len;
    }

    fn templatePart(idx: usize): JsTemplatePart? {
        if (idx >= _.parts_len) return null;
        return _.parts[idx];
    }

    fn knownPrimitive(): JsPrimitiveType {
        return _.known_prim;
    }

    fn isStringLike(): bool {
        return _.kind == JsFoldExprKind.StringLit || _.kind == JsFoldExprKind.InlinedEnumString;
    }

    fn isUntaggedTemplate(): bool {
        return _.kind == JsFoldExprKind.TemplateLit && !_.is_tagged;
    }

    /// Converts side-effect-free primitive literals (number, boolean, null, undefined)
    /// into a `StringLit`, mirroring Bun's `toStringExprWithoutSideEffects`.
    fn toStringExprWithoutSideEffects(): JsFoldExpr? {
        if (_.kind == JsFoldExprKind.StringLit) return _;
        if (_.kind == JsFoldExprKind.InlinedEnumString) {
            return JsFoldExpr.stringLit(_.strSlice());
        }
        if (_.kind == JsFoldExprKind.BoolLit) {
            return JsFoldExpr.stringLit(if (_.bool_val) "true" else "false");
        }
        if (_.kind == JsFoldExprKind.NullLit) {
            return JsFoldExpr.stringLit("null");
        }
        if (_.kind == JsFoldExprKind.UndefinedLit) {
            return JsFoldExpr.stringLit("undefined");
        }
        if (_.kind == JsFoldExprKind.NumberInt) {
            let num_buf: [24]u8 = [0; 24];
            const s = formatJsInt(_.int_val, num_buf[0..]) ?? return null;
            return JsFoldExpr.stringLit(s);
        }
        if (_.kind == JsFoldExprKind.TemplateLit && !_.is_tagged && _.parts_len == 0) {
            return JsFoldExpr.stringLit(_.templateHead());
        }
        return null;
    }

    /// Converts a simple untagged template literal with zero interpolations to a plain `StringLit`.
    fn simplifyTemplateToString(): JsFoldExpr? {
        if (_.kind == JsFoldExprKind.TemplateLit && !_.is_tagged && _.parts_len == 0) {
            return JsFoldExpr.stringLit(_.templateHead());
        }
        return null;
    }
}

fn cloneFoldExpr(src: JsFoldExpr): JsFoldExpr {
    return JsFoldExpr {
        kind: src.kind,
        str_buf: src.str_buf,
        str_len: src.str_len,
        int_val: src.int_val,
        bool_val: src.bool_val,
        expr_id: src.expr_id,
        known_prim: src.known_prim,
        is_tagged: src.is_tagged,
        prefer_template: src.prefer_template,
        parts: src.parts,
        parts_len: src.parts_len,
    };
}

fn unwrapInlined(e: JsFoldExpr): JsFoldExpr {
    if (e.kind == JsFoldExprKind.InlinedEnumString) {
        let copy = cloneFoldExpr(e);
        copy.kind = JsFoldExprKind.StringLit;
        return copy;
    }
    return cloneFoldExpr(e);
}

/// Folds `lhs + rhs` at compile time following Bun's `foldStringAddition` rules.
/// Returns `null` when the addition cannot be safely folded or exceeds fixed buffer bounds.
pub fn foldStringAddition(l: JsFoldExpr, r: JsFoldExpr, kind: FoldStringKind): JsFoldExpr? {
    let lhs = unwrapInlined(l);
    let rhs = unwrapInlined(r);

    // Normal mode: if RHS is string or untagged template, coerce side-effect-free LHS to string.
    // NestedLeft mode skips this to avoid mis-folding `(a + 1) + "y"`.
    if (kind != FoldStringKind.NestedLeft) {
        if (rhs.kind == JsFoldExprKind.StringLit || rhs.isUntaggedTemplate()) {
            if (coerced := lhs.toStringExprWithoutSideEffects()) {
                lhs = coerced;
            }
        }
    }

    if (lhs.kind == JsFoldExprKind.StringLit) {
        if (coerced_r := rhs.toStringExprWithoutSideEffects()) {
            rhs = coerced_r;
        }

        // Case 1: "bar" + "baz" => "barbaz"
        if (rhs.kind == JsFoldExprKind.StringLit) {
            let out = JsFoldExpr {
                kind: JsFoldExprKind.StringLit,
                known_prim: JsPrimitiveType.String,
                prefer_template: lhs.prefer_template || rhs.prefer_template,
            };
            const n = concatIntoBuf(out.str_buf[0..], lhs.strSlice(), rhs.strSlice()) ?? return null;
            out.str_len = n;
            return out;
        }

        // Case 2: "bar" + `baz${x}` => `barbaz${x}` (untagged templates only)
        if (rhs.isUntaggedTemplate()) {
            let out = cloneFoldExpr(rhs);
            const n = concatIntoBuf(out.str_buf[0..], lhs.strSlice(), rhs.templateHead()) ?? return null;
            out.str_len = n;
            out.prefer_template = true;
            return out;
        }

        // Case 3: "" + expr => expr (only when expr is guaranteed to be a string primitive)
        if (lhs.str_len == 0 && rhs.knownPrimitive() == JsPrimitiveType.String) {
            return cloneFoldExpr(rhs);
        }

        return null;
    }

    if (lhs.isUntaggedTemplate()) {
        // "`${x}` + 0" => "`${x}` + '0'"
        if (coerced_r := rhs.toStringExprWithoutSideEffects()) {
            rhs = coerced_r;
        }

        // Case 4: `foo${x}` + "baz" => `foo${x}baz`
        if (rhs.kind == JsFoldExprKind.StringLit) {
            let out = cloneFoldExpr(lhs);
            if (out.parts_len > 0) {
                const last_idx = out.parts_len - 1;
                if (!out.parts[last_idx].appendTail(rhs.strSlice())) return null;
                return out;
            } else {
                const n = concatIntoBuf(out.str_buf[0..], lhs.templateHead(), rhs.strSlice()) ?? return null;
                out.str_len = n;
                return out;
            }
        }

        // Case 5: `foo${x}` + `a${y}b` => `foo${x}a${y}b`
        if (rhs.isUntaggedTemplate()) {
            let out = cloneFoldExpr(lhs);
            if (out.parts_len > 0) {
                if (out.parts_len + rhs.parts_len > FOLD_MAX_TEMPLATE_PARTS) return null;
                const last_idx = out.parts_len - 1;
                if (!out.parts[last_idx].appendTail(rhs.templateHead())) return null;
                let k: usize = 0;
                while (k < rhs.parts_len) {
                    out.parts[out.parts_len] = rhs.parts[k];
                    out.parts_len += 1;
                    k += 1;
                }
                return out;
            } else {
                const n = concatIntoBuf(out.str_buf[0..], lhs.templateHead(), rhs.templateHead()) ?? return null;
                out.str_len = n;
                out.parts_len = rhs.parts_len;
                let k: usize = 0;
                while (k < rhs.parts_len) {
                    out.parts[k] = rhs.parts[k];
                    k += 1;
                }
                return out;
            }
        }
    }

    // Case 6: expr + "" => expr (only when lhs is guaranteed to be a string primitive)
    if (rhs.kind == JsFoldExprKind.StringLit && rhs.str_len == 0 && lhs.knownPrimitive() == JsPrimitiveType.String) {
        return cloneFoldExpr(lhs);
    }

    return null;
}
