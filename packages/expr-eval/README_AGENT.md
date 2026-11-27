# expr-eval@1.0.0 (Agent Quickref)

> Zero-allocation recursive-descent expression evaluation engine with variables, ternary/logical/math operators, built-in functions, and recursion depth guard for CP | License: MIT
> Import: `import { ... } from "expr-eval";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ExprError`
- `export enum TokenKind`

## Functions
- `export fn isSpace(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isIdentStart(c: u8): bool;`
- `export fn isIdentCont(c: u8): bool;`
- `export fn isTruthy(v: f64): bool;`
- `export fn parseFloatLiteral(slice: string): f64?;`
- `export fn evalExpr(source: string): EvalResult;`
- `export fn evalExprWith(source: string, env: ExprEnv): EvalResult;`
- `export fn evalBool(source: string, env: ExprEnv): bool;`
