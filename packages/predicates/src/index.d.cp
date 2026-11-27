// Type definitions for CP

import { strEq, eqIgnoreCase, startsWith, endsWith } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
export fn prdEqIgnoreCase(a: string, b: string): bool;
export fn prdIsFinite(x: f64): bool;
export enum IntPredKind {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    InRange,
    Always,
    Never,
    OpAnd,
    OpOr,
    OpNot
}

export class IntPredOp {
    kind: IntPredKind,
    val: i64,
    hi: i64,
    fn empty(): IntPredOp;
}

export class IntPredicate {
    ops: [16]IntPredOp,
    len: usize,
    fn fromOp(op: IntPredOp): IntPredicate;
    fn eq(v: i64): IntPredicate;
    fn ne(v: i64): IntPredicate;
    fn lt(v: i64): IntPredicate;
    fn le(v: i64): IntPredicate;
    fn gt(v: i64): IntPredicate;
    fn ge(v: i64): IntPredicate;
    fn inRange(lo: i64, hi: i64): IntPredicate;
    fn always(): IntPredicate;
    fn never(): IntPredicate;
    fn andPred(other: IntPredicate): IntPredicate;
    fn orPred(other: IntPredicate): IntPredicate;
    fn notPred(): IntPredicate;
    fn eval(x: i64): bool;
}

export enum StrPredKind {
    Eq,
    Contains,
    HasPrefix,
    HasSuffix,
    IsEmpty,
    LenEq,
    LenBetween,
    EqIgnoreCase,
    Always,
    Never,
    OpAnd,
    OpOr,
    OpNot
}

export class StrPredOp {
    kind: StrPredKind,
    target: string,
    lo: usize,
    hi: usize,
    fn empty(): StrPredOp;
}

export class StrPredicate {
    ops: [16]StrPredOp,
    len: usize,
    fn fromOp(op: StrPredOp): StrPredicate;
    fn eq(target: string): StrPredicate;
    fn contains(needle: string): StrPredicate;
    fn hasPrefix(prefix: string): StrPredicate;
    fn hasSuffix(suffix: string): StrPredicate;
    fn isEmpty(): StrPredicate;
    fn lenEq(n: usize): StrPredicate;
    fn lenBetween(lo: usize, hi: usize): StrPredicate;
    fn eqIgnoreCase(target: string): StrPredicate;
    fn always(): StrPredicate;
    fn never(): StrPredicate;
    fn andPred(other: StrPredicate): StrPredicate;
    fn orPred(other: StrPredicate): StrPredicate;
    fn notPred(): StrPredicate;
    fn eval(s: string): bool;
}

export enum FloatPredKind {
    ApproxEq,
    InRange,
    IsFinite,
    Always,
    Never,
    OpAnd,
    OpOr,
    OpNot
}

export class FloatPredOp {
    kind: FloatPredKind,
    target: f64,
    epsilon: f64,
    lo: f64,
    hi: f64,
    fn empty(): FloatPredOp;
}

export class FloatPredicate {
    ops: [16]FloatPredOp,
    len: usize,
    fn fromOp(op: FloatPredOp): FloatPredicate;
    fn approxEq(target: f64, epsilon: f64): FloatPredicate;
    fn inRange(lo: f64, hi: f64): FloatPredicate;
    fn isFinite(): FloatPredicate;
    fn always(): FloatPredicate;
    fn never(): FloatPredicate;
    fn andPred(other: FloatPredicate): FloatPredicate;
    fn orPred(other: FloatPredicate): FloatPredicate;
    fn notPred(): FloatPredicate;
    fn eval(x: f64): bool;
}

