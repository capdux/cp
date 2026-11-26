// Type definitions for CP

export enum DynValueKind {
    Undefined,
    Null,
    Boolean,
    Number,
    Int,
    Float,
    String,
    Array,
    Object,
    Function
}

export class DynValue {
    kind: DynValueKind,
    elem_kind: DynValueKind,
    array_len: usize,
    fn of(kind: DynValueKind): DynValue;
    fn arrayOf(elem_kind: DynValueKind, len: usize): DynValue;
}

export fn tckTrim(s: string): string;
export fn tckStrEq(a: string, b: string): bool;
export fn tckStartsWith(s: string, prefix: string): bool;
export fn tckMatchAtom(atom: string, val: DynValue): bool;
export fn typeCheck(type_expr: string, val: DynValue): bool;
export fn isValidTypeExpr(type_expr: string): bool;
