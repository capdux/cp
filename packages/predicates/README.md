[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# predicates

Zero-heap composable boolean predicate library for CP, modeled on Rust's [`predicates`](https://github.com/assert-rs/predicates-rs) crate (MIT OR Apache-2.0, CP v1.0.0).

Provides stack-allocated, composable `IntPredicate` (`i64`), `StrPredicate` (`string`), and `FloatPredicate` (`f64`) matchers backed by fixed 16-slot RPN instruction buffers—ideal for unit test assertions, input validation, and rule engines without dynamic dispatch or heap allocations.

## Installation & Import

```bash
cap add predicates
```

```cp
import { IntPredicate, StrPredicate, FloatPredicate } from "predicates";
```

## Core API Reference

### `IntPredicate` (`i64`)

| Method | Signature | Description |
| :--- | :--- | :--- |
| `eq` / `ne` / `lt` / `le` / `gt` / `ge` | `(v: i64): IntPredicate` | Standard integer comparison predicates. |
| `inRange(lo, hi)` | `(lo: i64, hi: i64): IntPredicate` | Inclusive range check `lo <= x && x <= hi`. |
| `always()` / `never()` | `(): IntPredicate` | Constant `true` / `false` predicates. |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): IntPredicate` | Boolean combinators composing predicates into a new value. |
| `eval(x)` | `(x: i64): bool` | Evaluates the composed predicate against `x`. |
| `fromOp(op)` | `(op: IntPredOp): IntPredicate` | Constructs a predicate from a single `IntPredOp`. |

### `StrPredicate` (`string`)

| Method | Signature | Description |
| :--- | :--- | :--- |
| `eq(target)` / `eqIgnoreCase(target)` | `(target: string): StrPredicate` | Exact or ASCII case-insensitive string equality. |
| `contains(needle)` | `(needle: string): StrPredicate` | Substring search matcher. |
| `hasPrefix(prefix)` / `hasSuffix(suffix)` | `(string): StrPredicate` | Prefix / suffix matcher. |
| `isEmpty()` | `(): StrPredicate` | Matches empty strings (`s.len == 0`). |
| `lenEq(n)` / `lenBetween(lo, hi)` | `(usize)` / `(usize, usize)` | Exact or inclusive byte-length matcher. |
| `always()` / `never()` | `(): StrPredicate` | Constant `true` / `false` predicates. |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): StrPredicate` | Boolean combinators. |
| `eval(s)` | `(s: string): bool` | Evaluates the composed predicate against `s`. |

### `FloatPredicate` (`f64`)

| Method | Signature | Description |
| :--- | :--- | :--- |
| `approxEq(target, epsilon)` | `(target: f64, epsilon: f64): FloatPredicate` | Checks `|x - target| <= epsilon` (rejects NaN). |
| `inRange(lo, hi)` | `(lo: f64, hi: f64): FloatPredicate` | Inclusive range check `lo <= x && x <= hi`. |
| `isFinite()` | `(): FloatPredicate` | Matches finite IEEE-754 floats (excludes NaN and ±Inf). |
| `always()` / `never()` | `(): FloatPredicate` | Constant `true` / `false` predicates. |
| `andPred(other)` / `orPred(other)` / `notPred()` | `(...): FloatPredicate` | Boolean combinators. |
| `eval(x)` | `(x: f64): bool` | Evaluates the composed predicate against `x`. |

### Helper Exports

`prdEqIgnoreCase(a: string, b: string): bool`, `prdIsFinite(x: f64): bool`, plus the low-level opcode types `IntPredKind`, `IntPredOp`, `StrPredKind`, `StrPredOp`, `FloatPredKind`, `FloatPredOp`.

## Usage Examples

### 1. Composing Integer Port & Range Rules

```cp
import { IntPredicate } from "predicates";

fn checkPort(port: i64): bool {
    let valid_user_port = IntPredicate.inRange(1024, 65535)
        .andPred(IntPredicate.ne(8080));
    let allow_http = valid_user_port.orPred(IntPredicate.eq(80));
    return allow_http.eval(port);
}
```

### 2. String Validation with Multiple Combinators

```cp
import { StrPredicate } from "predicates";

fn validateApiKey(key: string): bool {
    let rule = StrPredicate.hasPrefix("cp_")
        .andPred(StrPredicate.lenBetween(12, 64))
        .andPred(StrPredicate.contains(" ").notPred());
    return rule.eval(key);
}
```

### 3. Floating-Point Tolerance Assertions

```cp
import { FloatPredicate } from "predicates";

fn checkMetric(v: f64): bool {
    let rule = FloatPredicate.isFinite()
        .andPred(FloatPredicate.approxEq(1.0, 0.001));
    return rule.eval(v);
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Zero Heap & No Trait Objects**: Rust's `predicates` uses generic combinator trees or `Box<dyn Predicate<T>>`; CP's `predicates` compiles combinators into a flat 16-op stack-allocated RPN program, eliminating heap allocation and vtable indirection.
- **Zero-Copy String Matching**: `StrPredicate` stores borrowed `string` targets directly without cloning bytes.
- **IEEE-754 Safe Float Predicates**: `prdIsFinite` inspects raw IEEE-754 exponent bits via `@bitCast`, ensuring deterministic NaN/Inf handling.
