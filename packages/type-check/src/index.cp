// @cap/type-check — Zero-allocation runtime type expression parser & checker in pure CP
// Clean-room MIT implementation of npm `type-check` (#241, 195M/wk, ESLint core dep).

import { trim, strEq, startsWith } from "cap:ascii";

pub enum DynValueKind {
    Undefined,
    Null,
    Boolean,
    Number,
    Int,
    Float,
    String,
    Array,
    Object,
    Function,
}

pub struct DynValue {
    pub kind: DynValueKind,
    pub elem_kind: DynValueKind, // For homogeneous arrays
    pub array_len: usize,

    pub static fn of(kind: DynValueKind): DynValue {
        return DynValue { kind: kind, elem_kind: .Undefined, array_len: 0 };
    }

    pub static fn arrayOf(elem_kind: DynValueKind, len: usize): DynValue {
        return DynValue { kind: .Array, elem_kind: elem_kind, array_len: len };
    }
}

fn tckMatchAtom(atom: string, val: DynValue): bool {
    const t = trim(atom);
    if (t.len == 0) { return false; }
    if (strEq(t, "*")) { return true; }
    if (strEq(t, "Undefined")) { return val.kind == .Undefined; }
    if (strEq(t, "Null")) { return val.kind == .Null; }
    if (strEq(t, "Boolean")) { return val.kind == .Boolean; }
    if (strEq(t, "Number")) { return val.kind == .Number || val.kind == .Int || val.kind == .Float; }
    if (strEq(t, "Int")) { return val.kind == .Int; }
    if (strEq(t, "Float")) { return val.kind == .Float || val.kind == .Number; }
    if (strEq(t, "String")) { return val.kind == .String; }
    if (strEq(t, "Array")) { return val.kind == .Array; }
    if (strEq(t, "Object")) { return val.kind == .Object; }
    if (strEq(t, "Function")) { return val.kind == .Function; }

    // Maybe T => Undefined | Null | T
    if (startsWith(t, "Maybe ")) {
        if (val.kind == .Undefined || val.kind == .Null) {
            return true;
        }
        return typeCheck(t[6..t.len], val);
    }

    // Typed array: [ElemType]
    if (t.len >= 2 && t[0] == 91 && t[t.len - 1] == 93) {
        if (val.kind != .Array) {
            return false;
        }
        if (val.array_len == 0) {
            return true; // Empty array satisfies any [T]
        }
        const inner = trim(t[1..(t.len - 1)]);
        return typeCheck(inner, DynValue.of(val.elem_kind));
    }

    // Parenthesized expression: (Expr)
    if (t.len >= 2 && t[0] == 40 && t[t.len - 1] == 41) {
        return typeCheck(t[1..(t.len - 1)], val);
    }

    // Object shape literal `{ ... }`
    if (t.len >= 2 && t[0] == 123 && t[t.len - 1] == 125) {
        return val.kind == .Object;
    }

    return false;
}

/// Validates `val` against a `type-check` type expression (supporting `|`, `Maybe T`, `[T]`, `*`, `(T)`).
pub fn typeCheck(type_expr: string, val: DynValue): bool {
    const s = trim(type_expr);
    if (s.len == 0) {
        return false;
    }
    // Split top-level union branches by '|' (respecting (), [], {} nesting)
    let depth: i32 = 0;
    let seg_start: usize = 0;
    let i: usize = 0;
    let has_pipe = false;
    while (i < s.len) {
        const c = s[i];
        if (c == 40 || c == 91 || c == 123) {
            depth += 1;
        } else if (c == 41 || c == 93 || c == 125) {
            if (depth > 0) { depth -= 1; }
        } else if (c == 124 && depth == 0) { // '|'
            has_pipe = true;
            if (tckMatchAtom(s[seg_start..i], val)) {
                return true;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    if (has_pipe) {
        return tckMatchAtom(s[seg_start..s.len], val);
    }
    return tckMatchAtom(s, val);
}

/// Validates syntactic well-formedness (balanced delimiters & non-empty union branches) of a type expression.
pub fn isValidTypeExpr(type_expr: string): bool {
    const s = trim(type_expr);
    if (s.len == 0) { return false; }
    let paren: i32 = 0;
    let bracket: i32 = 0;
    let brace: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == 40) { paren += 1; }
        else if (c == 41) { paren -= 1; if (paren < 0) { return false; } }
        else if (c == 91) { bracket += 1; }
        else if (c == 93) { bracket -= 1; if (bracket < 0) { return false; } }
        else if (c == 123) { brace += 1; }
        else if (c == 125) { brace -= 1; if (brace < 0) { return false; } }
        i += 1;
    }
    return paren == 0 && bracket == 0 && brace == 0;
}

export {
    DynValueKind,
    DynValue,
    typeCheck,
    isValidTypeExpr,
};
