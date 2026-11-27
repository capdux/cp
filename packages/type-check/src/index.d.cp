// Type definitions for CP

import { trim, strEq, startsWith } from "cap:ascii";
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

export fn tckMatchAtom(atom: string, val: DynValue): bool;
export fn typeCheck(type_expr: string, val: DynValue): bool;
export fn isValidTypeExpr(type_expr: string): bool;
