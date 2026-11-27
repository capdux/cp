# predicates — AI Agent Quick Ref

Zero-heap composable boolean predicates (`IntPredicate`, `StrPredicate`, `FloatPredicate`) backed by 16-op stack RPN arrays (Rust `predicates` parity).

## Import

```cp
import {
    IntPredicate,
    StrPredicate,
    FloatPredicate,
    prdEqIgnoreCase,
    prdIsFinite,
} from "predicates";
```

## Core API Signatures

```cp
// IntPredicate (i64, ops: [16]IntPredOp)
IntPredicate.eq/ne/lt/le/gt/ge(v: i64): IntPredicate
IntPredicate.inRange(lo: i64, hi: i64): IntPredicate
IntPredicate.always(): IntPredicate; IntPredicate.never(): IntPredicate
p.andPred(other: IntPredicate): IntPredicate
p.orPred(other: IntPredicate): IntPredicate
p.notPred(): IntPredicate
p.eval(x: i64): bool

// StrPredicate (string, ops: [16]StrPredOp)
StrPredicate.eq(target: string): StrPredicate
StrPredicate.eqIgnoreCase(target: string): StrPredicate
StrPredicate.contains(needle: string): StrPredicate
StrPredicate.hasPrefix(prefix: string): StrPredicate
StrPredicate.hasSuffix(suffix: string): StrPredicate
StrPredicate.isEmpty(): StrPredicate
StrPredicate.lenEq(n: usize): StrPredicate
StrPredicate.lenBetween(lo: usize, hi: usize): StrPredicate
StrPredicate.always()/never(): StrPredicate
p.andPred/orPred(other: StrPredicate): StrPredicate; p.notPred(): StrPredicate
p.eval(s: string): bool

// FloatPredicate (f64, ops: [16]FloatPredOp)
FloatPredicate.approxEq(target: f64, epsilon: f64): FloatPredicate
FloatPredicate.inRange(lo: f64, hi: f64): FloatPredicate
FloatPredicate.isFinite(): FloatPredicate
FloatPredicate.always()/never(): FloatPredicate
p.andPred/orPred(other: FloatPredicate): FloatPredicate; p.notPred(): FloatPredicate
p.eval(x: f64): bool
```

## Minimal Example

```cp
import { IntPredicate, StrPredicate } from "predicates";

fn demo(): bool {
    let ok_num = IntPredicate.inRange(1, 100).andPred(IntPredicate.ne(42)).eval(50);
    let ok_str = StrPredicate.hasPrefix("ok:").andPred(StrPredicate.lenBetween(4, 16)).eval("ok:done");
    return ok_num && ok_str;
}
```

## Gotchas & Borrow Rules

- **16-op stack limit**: Each predicate holds `[16]*PredOp`; combining predicates via `andPred`/`orPred`/`notPred` appends ops (`len_a + len_b + 1 <= 16`). Keep composed expressions within 16 total ops.
- **Value semantics**: All combinators return a new predicate value; `self` is never mutated.
- **String lifetime**: `StrPredicate` stores `string` slices directly; ensure target strings outlive `eval(...)`.
- **ASCII case fold**: `eqIgnoreCase` / `prdEqIgnoreCase` uses ASCII case-insensitive comparison.
