# expr-eval@1.0.0 — AI Agent Quick Ref

> Zero-alloc recursive-descent math/logic expression evaluator with stack `ExprEnv` & depth guard | Source: `npm:expr-eval` | License: MIT
> Import: `import { ExprEnv, ExprError, EvalResult, evalExpr, evalExprWith, evalBool } from "expr-eval";` | Types: `src/index.d.cp`

## Core Types & Constants

```cp
export enum ExprError {
    None = 0, SyntaxError = 1, UnexpectedToken = 2, DivisionByZero = 3,
    UnknownVariable = 4, UnknownFunction = 5, InvalidArgs = 6, MaxDepthExceeded = 7
}

export class EvalResult {
    ok: bool, value: f64, err: ExprError, pos: usize,
    fn success(v: f64): EvalResult;
    fn fail(err: ExprError, pos: usize): EvalResult;
}

export class ExprEnv {
    entries: [64]EnvEntry, count: usize,
    fn init(): ExprEnv;
    fn set(_: auto, name: string, value: f64);
    fn get(name: string): f64?;
    fn has(name: string): bool;
    fn clear(_: auto);
    fn len(): usize;
}
// Constants: MAX_VARS=64, MAX_DEPTH=32, MAX_CALL_ARGS=8, MATH_PI, MATH_E
```

## Core Functions

- `evalExpr(source: string): EvalResult` — Evaluate with empty env.
- `evalExprWith(source: string, env: ExprEnv): EvalResult` — Evaluate with bound variables.
- `evalBool(source: string, env: ExprEnv): bool` — Returns `true` iff `ok && isTruthy(value)` (errors yield `false`).
- `isTruthy(v: f64): bool` | `parseFloatLiteral(slice: string): f64?`

## Built-ins & Operators

- **Precedence (low -> high)**: `?:` -> `||` -> `&&` -> `== !=` -> `< <= > >=` -> `+ -` -> `* / %` -> `^` (right-assoc) -> unary `- + !` -> atoms.
- **Literals/Constants**: numbers, `true` (`1.0`), `false` (`0.0`), `PI`, `E`.
- **12 Functions**: `abs(x)`, `min(a,b)`, `max(a,b)`, `floor(x)`, `ceil(x)`, `round(x)`, `sqrt(x)`, `pow(a,b)`, `clamp(x,lo,hi)`, `sign(x)`, `avg(a,b)`, `if(c,a,b)`.

## Minimal Usage

```cp
import { ExprEnv, evalExprWith, evalBool } from "expr-eval";

let env = ExprEnv.init();
env.set("x", 10.0);
const res = evalExprWith("clamp(x * 2, 0, 15)", env); // res.ok == true, res.value == 15.0
const ok = evalBool("x > 5 && x < 20", env);          // true
```

## Pitfalls & Rules

1. **All values are `f64`**: Comparisons and booleans evaluate to `1.0` / `0.0`; strings are not expression values.
2. **Limits**: Max 64 vars in `ExprEnv` (extra `set` ignored if full), max 8 args per call, max recursion depth `32` (`MaxDepthExceeded`).
3. **Domain checks**: Division/modulo by `0` -> `DivisionByZero`; `sqrt(x<0)` or `clamp` with `lo > hi` -> `InvalidArgs`.
4. **Mutable `ExprEnv`**: Declare `let env = ExprEnv.init();` before calling `env.set(...)` or `env.clear()`.
