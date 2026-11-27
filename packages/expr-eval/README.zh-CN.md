[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# expr-eval

纯 CP 零堆分配递归下降数学与逻辑表达式求值引擎 —— 洁净室 MIT 实现，对标 npm `expr-eval`。

支持四则运算、比较、短路逻辑（`&&`/`||`）、三元表达式（`cond ? a : b`）、右结合幂运算（`^`）、栈上命名 `f64` 变量表（最多 64 个）、内置常量（`PI`/`E`/`true`/`false`）与 12 个安全内置函数，提供结构化错误码（`EvalResult` + `ExprError`）及递归深度护栏（`MAX_DEPTH = 32`）。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `expr-eval`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/silentmatt/expr-eval` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: expr-eval)
import { Parser } from "expr-eval";
const value = Parser.evaluate("round(price * qty * (1 - discount))", { price: 120, qty: 3, discount: 0.15 });
```

```cp
// CP (cap: expr-eval) — 零堆分配栈上 ExprEnv 与结构化 EvalResult
import { ExprEnv, evalExprWith } from "expr-eval";

let env = ExprEnv.init();
env.set("price", 120.0);
env.set("qty", 3.0);
env.set("discount", 0.15);
const res = evalExprWith("round(price * qty * (1 - discount))", env);
// res.ok == true, res.value == 306.0
```

## 安装

```bash
cap add expr-eval
```

## 使用示例

### 基础用法：直接求值、变量绑定与布尔规则判定

```cp
import { ExprEnv, evalExpr, evalExprWith, evalBool } from "expr-eval";

fn demoEval() {
    // 1. 直接求值（支持运算符优先级与右结合幂运算 2 ^ (3 ^ 2) == 512）
    const r1 = evalExpr("(2 + 3) * 4 - 2 ^ 3 ^ 2");
    // r1.ok == true, r1.value == -492.0

    // 2. 栈上变量绑定与动态定价公式
    let env = ExprEnv.init();
    env.set("price", 120.0);
    env.set("qty", 3.0);
    env.set("discount", 0.15);

    const total = evalExprWith("round(price * qty * (1 - discount))", env);
    // total.ok == true, total.value == 306.0

    // 3. 布尔规则判定（支持短路逻辑）
    const is_vip = evalBool("price * qty >= 300 && discount <= 0.2", env);
    // is_vip == true
    _ = r1;
    _ = total;
    _ = is_vip;
}
```

### 边界用法：短路保护、定义域错误与深度护栏

```cp
import { ExprEnv, ExprError, evalExpr, evalExprWith } from "expr-eval";

fn demoEdgeCases() {
    let env = ExprEnv.init();
    env.set("x", 0.0);

    // 1. 三元/逻辑短路跳过未激活分支，安全避开除零
    const safe = evalExprWith("x != 0 ? 100 / x : 0", env);
    // safe.ok == true, safe.value == 0.0

    // 2. 结构化错误码与精确字节偏移 pos（无异常抛错）
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

## 内置函数一览（12 个）

`abs(x)`、`min(a, b)`、`max(a, b)`、`floor(x)`、`ceil(x)`、`round(x)`、`sqrt(x)`、`pow(a, b)`、`clamp(x, lo, hi)`、`sign(x)`、`avg(a, b)`、`if(cond, a, b)`。

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `ExprError` | `enum { None, SyntaxError, UnexpectedToken, DivisionByZero, UnknownVariable, UnknownFunction, InvalidArgs, MaxDepthExceeded }` | 结构化求值错误码 |
| `EvalResult` | `class { ok: bool, value: f64, err: ExprError, pos: usize }` | 求值结果，含 `value` 与错误字节偏移 `pos` |
| `ExprEnv` | `class { entries: [64]EnvEntry, count: usize }` | 栈上变量表（`init()`、`set(*_, name, val)`、`get(name): f64?`、`has(name): bool`、`clear(*_)`、`len(): usize`） |
| `evalExpr` | `fn(source: string): EvalResult` | 在空环境下直接求值 |
| `evalExprWith` | `fn(source: string, env: ExprEnv): EvalResult` | 在给定变量环境 `env` 下求值 |
| `evalBool` | `fn(source: string, env: ExprEnv): bool` | 求值并判定真值（求值出错时安全返回 `false`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isSpace`、`isDigit`、`isAlpha`、`isAlnum`、`strEq`）实现高速单遍词法分析与内置函数分发。
- **零堆分配与固定栈池**：`ExprEnv` 栈上固定 `[64]EnvEntry`，函数实参使用栈上 `[8]f64`，无任何堆分配。
- **`undefined` 零清零优化**：变量池与实参数组均采用 `undefined` 声明，省去多余内存清零开销。
- **递归深度护栏**：内置 `MAX_DEPTH = 32` 防护，超深嵌套表达式确定性返回 `ExprError.MaxDepthExceeded`。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `expr-eval`。
