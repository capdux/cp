[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# mathjs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mathjs` ([https://github.com/josdejong/mathjs](https://github.com/josdejong/mathjs))（原库名 `mathjs` → CP 包名 `mathjs`）

Zero-allocation mathematical expression parser & evaluator for CP — clean-room MIT parity with npm `mathjs` expression evaluation. Composes `libm` with zero duplication.

纯 CP 零堆分配数学表达式解析与求值引擎（对标 npm `mathjs`），复用 `libm` 提供三角、指数、对数、幂与取整函数，支持自定义变量作用域、内置数学常量与严格错误码检测。

## Install / 安装

```bash
cap add mathjs
```

## Dependencies / 跨包复用

- `libm`: IEEE 754 `f64` 初等与超越数学函数（`sin` / `cos` / `tan` / `sqrt` / `exp` / `ln` / `pow` / `fabs` / `floor` / `ceil` / `round` / `fmod` / `fmin` / `fmax`）及高精度常量 `PI` / `TAU` / `E`

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MathScope` | `struct` + `.new` / `.set` / `.get` | Stack-allocated variable scope (up to 32 bindings) with built-in `pi`, `e`, `tau`, `phi` |
| `EvalResult` | `struct { ok: bool, value: f64, err_code: i32 }` | Result of expression evaluation |
| `evalExpr` | `(expr: string): EvalResult` | Parse & evaluate an expression with default built-in constants |
| `evalExprWithScope` | `(expr: string, scope: MathScope): EvalResult` | Parse & evaluate an expression with custom variable bindings |

### Built-in Functions (14)

`sin`, `cos`, `tan`, `sqrt`, `abs`, `floor`, `ceil`, `round`, `exp`, `log`, `pow(a, b)`, `min(a, b)`, `max(a, b)`, `clamp(x, lo, hi)`

## Usage / 使用示例

```cp
import { MathScope, evalExpr, evalExprWithScope } from "mathjs";

const r1 = evalExpr("2^3^2"); // 512.0
let scope = MathScope.new();
scope.set("x", 3.0);
scope.set("y", 4.0);
const r2 = evalExprWithScope("sqrt(x^2 + y^2)", scope); // 5.0
```
