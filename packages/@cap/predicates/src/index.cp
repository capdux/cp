// @cap/predicates — Zero-heap composable boolean predicates for CP
// Ported from crates.io:predicates (https://github.com/assert-rs/predicates-rs)

fn prdToLowerAscii(b: u8): u8 {
    if (b >= 0x41 && b <= 0x5A) return b + 32;
    return b;
}

fn prdStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn prdStrEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (prdToLowerAscii(a[i]) != prdToLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn prdStrHasPrefix(s: string, prefix: string): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn prdStrHasSuffix(s: string, suffix: string): bool {
    if (suffix.len > s.len) return false;
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[off + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn prdStrContains(s: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > s.len) return false;
    let i: usize = 0;
    const limit = s.len - needle.len;
    while (i <= limit) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) {
            if (s[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

fn prdIsFinite(x: f64): bool {
    const bits = @as(u64, @bitCast(x));
    const exp: u64 = (bits >> 52) & 0x7FF;
    return exp != 0x7FF;
}

// ============================================================
// IntPredicate
// ============================================================

pub enum IntPredKind {
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
    OpNot,
}

pub struct IntPredOp {
    kind: IntPredKind,
    val: i64,
    hi: i64,

    static fn empty(): IntPredOp {
        return IntPredOp{
            kind: IntPredKind.Always,
            val: 0,
            hi: 0,
        };
    }
}

pub struct IntPredicate {
    ops: [16]IntPredOp,
    len: usize,

    static fn fromOp(op: IntPredOp): IntPredicate {
        const e = IntPredOp.empty();
        let arr: [16]IntPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        arr[0] = op;
        return IntPredicate{ ops: arr, len: 1 };
    }

    static fn eq(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Eq, val: v, hi: 0 });
    }

    static fn ne(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Ne, val: v, hi: 0 });
    }

    static fn lt(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Lt, val: v, hi: 0 });
    }

    static fn le(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Le, val: v, hi: 0 });
    }

    static fn gt(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Gt, val: v, hi: 0 });
    }

    static fn ge(v: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Ge, val: v, hi: 0 });
    }

    static fn inRange(lo: i64, hi: i64): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.InRange, val: lo, hi: hi });
    }

    static fn always(): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Always, val: 0, hi: 0 });
    }

    static fn never(): IntPredicate {
        return IntPredicate.fromOp(IntPredOp{ kind: IntPredKind.Never, val: 0, hi: 0 });
    }

    fn andPred(other: IntPredicate): IntPredicate {
        const e = IntPredOp.empty();
        let arr: [16]IntPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = IntPredOp{ kind: IntPredKind.OpAnd, val: 0, hi: 0 };
        n += 1;
        return IntPredicate{ ops: arr, len: n };
    }

    fn orPred(other: IntPredicate): IntPredicate {
        const e = IntPredOp.empty();
        let arr: [16]IntPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = IntPredOp{ kind: IntPredKind.OpOr, val: 0, hi: 0 };
        n += 1;
        return IntPredicate{ ops: arr, len: n };
    }

    fn notPred(): IntPredicate {
        const e = IntPredOp.empty();
        let arr: [16]IntPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = IntPredOp{ kind: IntPredKind.OpNot, val: 0, hi: 0 };
        n += 1;
        return IntPredicate{ ops: arr, len: n };
    }

    fn eval(x: i64): bool {
        let stack: [16]bool = [false; 16];
        let sp: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const op = _.ops[i];
            if (op.kind == IntPredKind.OpAnd) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a && b;
                }
            } else if (op.kind == IntPredKind.OpOr) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a || b;
                }
            } else if (op.kind == IntPredKind.OpNot) {
                if (sp >= 1) {
                    stack[sp - 1] = !stack[sp - 1];
                }
            } else {
                let res: bool = false;
                if (op.kind == IntPredKind.Eq) res = x == op.val;
                else if (op.kind == IntPredKind.Ne) res = x != op.val;
                else if (op.kind == IntPredKind.Lt) res = x < op.val;
                else if (op.kind == IntPredKind.Le) res = x <= op.val;
                else if (op.kind == IntPredKind.Gt) res = x > op.val;
                else if (op.kind == IntPredKind.Ge) res = x >= op.val;
                else if (op.kind == IntPredKind.InRange) res = x >= op.val && x <= op.hi;
                else if (op.kind == IntPredKind.Always) res = true;
                else res = false;
                if (sp < 16) {
                    stack[sp] = res;
                    sp += 1;
                }
            }
            i += 1;
        }
        if (sp == 0) return false;
        return stack[sp - 1];
    }
}

// ============================================================
// StrPredicate
// ============================================================

pub enum StrPredKind {
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
    OpNot,
}

pub struct StrPredOp {
    kind: StrPredKind,
    target: string,
    lo: usize,
    hi: usize,

    static fn empty(): StrPredOp {
        return StrPredOp{
            kind: StrPredKind.Always,
            target: "",
            lo: 0,
            hi: 0,
        };
    }
}

pub struct StrPredicate {
    ops: [16]StrPredOp,
    len: usize,

    static fn fromOp(op: StrPredOp): StrPredicate {
        const e = StrPredOp.empty();
        let arr: [16]StrPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        arr[0] = op;
        return StrPredicate{ ops: arr, len: 1 };
    }

    static fn eq(target: string): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.Eq, target: target, lo: 0, hi: 0 });
    }

    static fn contains(needle: string): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.Contains, target: needle, lo: 0, hi: 0 });
    }

    static fn hasPrefix(prefix: string): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.HasPrefix, target: prefix, lo: 0, hi: 0 });
    }

    static fn hasSuffix(suffix: string): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.HasSuffix, target: suffix, lo: 0, hi: 0 });
    }

    static fn isEmpty(): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.IsEmpty, target: "", lo: 0, hi: 0 });
    }

    static fn lenEq(n: usize): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.LenEq, target: "", lo: n, hi: n });
    }

    static fn lenBetween(lo: usize, hi: usize): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.LenBetween, target: "", lo: lo, hi: hi });
    }

    static fn eqIgnoreCase(target: string): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.EqIgnoreCase, target: target, lo: 0, hi: 0 });
    }

    static fn always(): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.Always, target: "", lo: 0, hi: 0 });
    }

    static fn never(): StrPredicate {
        return StrPredicate.fromOp(StrPredOp{ kind: StrPredKind.Never, target: "", lo: 0, hi: 0 });
    }

    fn andPred(other: StrPredicate): StrPredicate {
        const e = StrPredOp.empty();
        let arr: [16]StrPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = StrPredOp{ kind: StrPredKind.OpAnd, target: "", lo: 0, hi: 0 };
        n += 1;
        return StrPredicate{ ops: arr, len: n };
    }

    fn orPred(other: StrPredicate): StrPredicate {
        const e = StrPredOp.empty();
        let arr: [16]StrPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = StrPredOp{ kind: StrPredKind.OpOr, target: "", lo: 0, hi: 0 };
        n += 1;
        return StrPredicate{ ops: arr, len: n };
    }

    fn notPred(): StrPredicate {
        const e = StrPredOp.empty();
        let arr: [16]StrPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = StrPredOp{ kind: StrPredKind.OpNot, target: "", lo: 0, hi: 0 };
        n += 1;
        return StrPredicate{ ops: arr, len: n };
    }

    fn eval(s: string): bool {
        let stack: [16]bool = [false; 16];
        let sp: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const op = _.ops[i];
            if (op.kind == StrPredKind.OpAnd) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a && b;
                }
            } else if (op.kind == StrPredKind.OpOr) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a || b;
                }
            } else if (op.kind == StrPredKind.OpNot) {
                if (sp >= 1) {
                    stack[sp - 1] = !stack[sp - 1];
                }
            } else {
                let res: bool = false;
                if (op.kind == StrPredKind.Eq) res = prdStrEq(s, op.target);
                else if (op.kind == StrPredKind.Contains) res = prdStrContains(s, op.target);
                else if (op.kind == StrPredKind.HasPrefix) res = prdStrHasPrefix(s, op.target);
                else if (op.kind == StrPredKind.HasSuffix) res = prdStrHasSuffix(s, op.target);
                else if (op.kind == StrPredKind.IsEmpty) res = s.len == 0;
                else if (op.kind == StrPredKind.LenEq) res = s.len == op.lo;
                else if (op.kind == StrPredKind.LenBetween) res = s.len >= op.lo && s.len <= op.hi;
                else if (op.kind == StrPredKind.EqIgnoreCase) res = prdStrEqIgnoreCase(s, op.target);
                else if (op.kind == StrPredKind.Always) res = true;
                else res = false;
                if (sp < 16) {
                    stack[sp] = res;
                    sp += 1;
                }
            }
            i += 1;
        }
        if (sp == 0) return false;
        return stack[sp - 1];
    }
}

// ============================================================
// FloatPredicate
// ============================================================

pub enum FloatPredKind {
    ApproxEq,
    InRange,
    IsFinite,
    Always,
    Never,
    OpAnd,
    OpOr,
    OpNot,
}

pub struct FloatPredOp {
    kind: FloatPredKind,
    target: f64,
    epsilon: f64,
    lo: f64,
    hi: f64,

    static fn empty(): FloatPredOp {
        return FloatPredOp{
            kind: FloatPredKind.Always,
            target: 0.0,
            epsilon: 0.0,
            lo: 0.0,
            hi: 0.0,
        };
    }
}

pub struct FloatPredicate {
    ops: [16]FloatPredOp,
    len: usize,

    static fn fromOp(op: FloatPredOp): FloatPredicate {
        const e = FloatPredOp.empty();
        let arr: [16]FloatPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        arr[0] = op;
        return FloatPredicate{ ops: arr, len: 1 };
    }

    static fn approxEq(target: f64, epsilon: f64): FloatPredicate {
        return FloatPredicate.fromOp(FloatPredOp{
            kind: FloatPredKind.ApproxEq,
            target: target,
            epsilon: epsilon,
            lo: 0.0,
            hi: 0.0,
        });
    }

    static fn inRange(lo: f64, hi: f64): FloatPredicate {
        return FloatPredicate.fromOp(FloatPredOp{
            kind: FloatPredKind.InRange,
            target: 0.0,
            epsilon: 0.0,
            lo: lo,
            hi: hi,
        });
    }

    static fn isFinite(): FloatPredicate {
        return FloatPredicate.fromOp(FloatPredOp{
            kind: FloatPredKind.IsFinite,
            target: 0.0,
            epsilon: 0.0,
            lo: 0.0,
            hi: 0.0,
        });
    }

    static fn always(): FloatPredicate {
        return FloatPredicate.fromOp(FloatPredOp{
            kind: FloatPredKind.Always,
            target: 0.0,
            epsilon: 0.0,
            lo: 0.0,
            hi: 0.0,
        });
    }

    static fn never(): FloatPredicate {
        return FloatPredicate.fromOp(FloatPredOp{
            kind: FloatPredKind.Never,
            target: 0.0,
            epsilon: 0.0,
            lo: 0.0,
            hi: 0.0,
        });
    }

    fn andPred(other: FloatPredicate): FloatPredicate {
        const e = FloatPredOp.empty();
        let arr: [16]FloatPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = FloatPredOp{ kind: FloatPredKind.OpAnd, target: 0.0, epsilon: 0.0, lo: 0.0, hi: 0.0 };
        n += 1;
        return FloatPredicate{ ops: arr, len: n };
    }

    fn orPred(other: FloatPredicate): FloatPredicate {
        const e = FloatPredOp.empty();
        let arr: [16]FloatPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        i = 0;
        while (i < other.len && n < 15) {
            arr[n] = other.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = FloatPredOp{ kind: FloatPredKind.OpOr, target: 0.0, epsilon: 0.0, lo: 0.0, hi: 0.0 };
        n += 1;
        return FloatPredicate{ ops: arr, len: n };
    }

    fn notPred(): FloatPredicate {
        const e = FloatPredOp.empty();
        let arr: [16]FloatPredOp = [e, e, e, e, e, e, e, e, e, e, e, e, e, e, e, e];
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.len && n < 15) {
            arr[n] = _.ops[i];
            n += 1;
            i += 1;
        }
        arr[n] = FloatPredOp{ kind: FloatPredKind.OpNot, target: 0.0, epsilon: 0.0, lo: 0.0, hi: 0.0 };
        n += 1;
        return FloatPredicate{ ops: arr, len: n };
    }

    fn eval(x: f64): bool {
        let stack: [16]bool = [false; 16];
        let sp: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const op = _.ops[i];
            if (op.kind == FloatPredKind.OpAnd) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a && b;
                }
            } else if (op.kind == FloatPredKind.OpOr) {
                if (sp >= 2) {
                    const b = stack[sp - 1];
                    const a = stack[sp - 2];
                    sp -= 1;
                    stack[sp - 1] = a || b;
                }
            } else if (op.kind == FloatPredKind.OpNot) {
                if (sp >= 1) {
                    stack[sp - 1] = !stack[sp - 1];
                }
            } else {
                let res: bool = false;
                if (op.kind == FloatPredKind.ApproxEq) {
                    if (prdIsFinite(x) && prdIsFinite(op.target)) {
                        let diff = x - op.target;
                        if (diff < 0.0) diff = -diff;
                        res = diff <= op.epsilon;
                    }
                } else if (op.kind == FloatPredKind.InRange) {
                    res = prdIsFinite(x) && x >= op.lo && x <= op.hi;
                } else if (op.kind == FloatPredKind.IsFinite) {
                    res = prdIsFinite(x);
                } else if (op.kind == FloatPredKind.Always) {
                    res = true;
                } else {
                    res = false;
                }
                if (sp < 16) {
                    stack[sp] = res;
                    sp += 1;
                }
            }
            i += 1;
        }
        if (sp == 0) return false;
        return stack[sp - 1];
    }
}
