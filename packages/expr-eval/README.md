[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# expr-eval

Zero-allocation recursive-descent math and logic expression evaluator for CP — clean-room MIT parity with npm `expr-eval`.

Evaluates arithmetic, comparison, short-circuit boolean (`&&`/`||`), ternary (`cond ? a : b`), right-associative power (`^`), named `f64` variables (up to 64 in a stack `ExprEnv`), constants (`PI`, `E`, `true`, `false`), and 12 built-in functions with structured error diagnostics (`EvalResult` + `ExprError`) and a built-in recursion depth guard (`MAX_DEPTH = 32`).

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `expr-eval` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/silentmatt/expr-eval` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: expr-eval)
import { Parser } from "expr-eval";
const value = Parser.evaluate("round(price * qty * (1 - discount))", { price: 120, qty: 3, discount: 0.15 });
```

```cp
// CP (cap: expr-eval) — zero-allocation stack ExprEnv & structured EvalResult
import { ExprEnv, evalExprWith } from "expr-eval";

let env = ExprEnv.init();
env.set("price", 120.0);
env.set("qty", 3.0);
env.set("discount", 0.15);
const res = evalExprWith("round(price * qty * (1 - discount))", env);
// res.ok == true, res.value == 306.0
```

## Installation

```bash
cap add expr-eval
```

## Usage

### Basic Usage: Stateless Math, Variable Binding & Boolean Rules

```cp
import { ExprEnv, evalExpr, evalExprWith, evalBool } from "expr-eval";

fn demoEval() {
    // 1. Direct evaluation with precedence & right-associative power (2 ^ (3 ^ 2) == 512)
    const r1 = evalExpr("(2 + 3) * 4 - 2 ^ 3 ^ 2");
    // r1.ok == true, r1.value == -492.0

    // 2. Stack-allocated variable environment
    let env = ExprEnv.init();
    env.set("price", 120.0);
    env.set("qty", 3.0);
    env.set("discount", 0.15);

    const total = evalExprWith("round(price * qty * (1 - discount))", env);
    // total.ok == true, total.value == 306.0

    // 3. Boolean rule evaluation with short-circuit logic
    const is_vip = evalBool("price * qty >= 300 && discount <= 0.2", env);
    // is_vip == true
    _ = r1;
    _ = total;
    _ = is_vip;
}
```

### Edge Cases: Short-Circuiting, Domain Errors & Depth Guard

```cp
import { ExprEnv, ExprError, evalExpr, evalExprWith } from "expr-eval";

fn demoEdgeCases() {
    let env = ExprEnv.init();
    env.set("x", 0.0);

    // 1. Short-circuit avoids division by zero in inactive branch
    const safe = evalExprWith("x != 0 ? 100 / x : 0", env);
    // safe.ok == true, safe.value == 0.0

    // 2. Structured error codes with exact byte offset (no panics/exceptions)
    const div0 = evalExpr("10 / (5 - 5)");
    // div0.ok == false, div0.err == ExprError.DivisionByZero

    const bad_sqrt = evalExpr("sqrt(-4)");
    // bad_sqrt.ok == false, bad_sqrt.err == ExprError.InvalidArgs

    const unknown = evalExprWith("missing_var + 1", env);
    // unknown.ok == false, unknown.err == ExprError.UnknownVariable
    _ = safe;
    _ = div0;
    _ = bad_sqrt;
    _ = unknown;
}
```

## Built-in Functions (12)

`abs(x)`, `min(a, b)`, `max(a, b)`, `floor(x)`, `ceil(x)`, `round(x)`, `sqrt(x)`, `pow(a, b)`, `clamp(x, lo, hi)`, `sign(x)`, `avg(a, b)`, `if(cond, a, b)`.

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `ExprError` | `enum { None, SyntaxError, UnexpectedToken, DivisionByZero, UnknownVariable, UnknownFunction, InvalidArgs, MaxDepthExceeded }` | Structured error codes returned inside `EvalResult`. |
| `EvalResult` | `class { ok: bool, value: f64, err: ExprError, pos: usize }` | Evaluation outcome with `value` and exact failure byte offset `pos`. |
| `ExprEnv` | `class { entries: [64]EnvEntry, count: usize }` | Stack variable table (`init()`, `set(*_, name, val)`, `get(name): f64?`, `has(name): bool`, `clear(*_)`, `len(): usize`). |
| `evalExpr` | `fn(source: string): EvalResult` | Evaluates an expression with an empty environment. |
| `evalExprWith` | `fn(source: string, env: ExprEnv): EvalResult` | Evaluates an expression with bound variables in `env`. |
| `evalBool` | `fn(source: string, env: ExprEnv): bool` | Evaluates an expression and returns `true` iff `ok && isTruthy(value)` (`false` on error). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isDigit`, `isAlpha`, `isAlnum`, `strEq`) for fast single-pass lexing and built-in dispatch.
- **Zero Heap Allocation & Fixed Stack Pools**: `ExprEnv` holds up to 64 variables (`[64]EnvEntry`) and call arguments use a stack `[8]f64` array.
- **Zero-Clear Stack Arrays (`undefined`)**: Unoccupied variable slots and argument buffers use `undefined` initialization to eliminate memset overhead.
- **Recursion Depth Guard**: Enforces `MAX_DEPTH = 32` to defend against deeply nested parentheses causing stack overflow (`ExprError.MaxDepthExceeded`).

## License

MIT — Clean-room CP implementation inspired by npm `expr-eval`.
