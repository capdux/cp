[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/expr` — 零分配递归下降表达式求值引擎 / Zero-Allocation Expression Evaluation Engine

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:expr-eval` ([https://github.com/silentmatt/expr-eval](https://github.com/silentmatt/expr-eval))（原库名 `expr-eval` → CP 包名 `@cap/expr`）

> 独立 Clean-Room 纯 CP 实现，对标 npm `expr-eval` / `filtrex` / `jexl` 与 Rust `evalexpr` / `meval` | 许可协议：MIT

## 安装 / Installation

```bash
cap add @cap/expr
```

## 概述 / Overview

`@cap/expr` 是一个纯 CP、零外部依赖、零堆分配的递归下降数学与逻辑表达式求值引擎，适用于规则引擎、动态定价公式、告警阈值判定、配置条件过滤等高频低延迟场景：

- **零堆分配栈上上下文**：`ExprEnv` 在栈上最多绑定 64 个命名 `f64` 变量，支持 `init()`、`set(*_, name, value)`、`get(name) -> f64?`、`has(name) -> bool`；
- **结构化错误诊断**：无异常抛错，统一返回 `EvalResult{ ok, value, err, pos }`，附带精确字节偏移 `pos` 与 `ExprError` 错误码；
- **完整运算符与短路求值**：支持逻辑 `||` / `&&`、比较、四则、右结合幂运算 `^`、一元 `- + !`、三元 `cond ? a : b`、布尔字面量 `true`/`false` 与常量 `PI`/`E`；
- **12 个安全内置函数**：开箱即用的数学与条件函数，参数个数与域边界（如负数开方、`clamp` 上下界）均严格校验；
- **递归深度安全护栏**：内置 `MAX_DEPTH = 32`，抵御深层嵌套括号或恶意递归输入导致的栈溢出风险。

## 运算符与优先级 / Operators & Precedence

按优先级从低到高排列：

| 优先级 | 运算符 | 结合性 | 说明 |
| :---: | :--- | :--- | :--- |
| 1 | `cond ? a : b` | 右结合 | 三元条件表达式（支持短路） |
| 2 | `\|\|` | 左结合 | 逻辑或（支持短路） |
| 3 | `&&` | 左结合 | 逻辑与（支持短路） |
| 4 | `==` `!=` | 左结合 | 相等 / 不等比较（返回 `1.0` / `0.0`） |
| 5 | `<` `<=` `>` `>=` | 左结合 | 大小比较（返回 `1.0` / `0.0`） |
| 6 | `+` `-` | 左结合 | 加法 / 减法 |
| 7 | `*` `/` `%` | 左结合 | 乘法 / 除法 / 取余（除零返回 `DivisionByZero`） |
| 8 | `^` | 右结合 | 幂运算（如 `2 ^ 3 ^ 2 == 512`） |
| 9 | `-` `+` `!` | 右结合 | 一元负号 / 正号 / 逻辑取反 |
| 10 | `(...)` `fn(...)` | — | 括号分组、函数调用、数字、`true`/`false`、`PI`/`E`、变量 |

## 内置函数 / Built-in Functions

| 函数 | 参数 | 说明 |
| :--- | :---: | :--- |
| `abs(x)` | 1 | 绝对值 |
| `min(a, b)` | 2 | 较小值 |
| `max(a, b)` | 2 | 较大值 |
| `floor(x)` | 1 | 向下取整 |
| `ceil(x)` | 1 | 向上取整 |
| `round(x)` | 1 | 四舍五入 |
| `sqrt(x)` | 1 | 平方根（`x < 0` 返回 `InvalidArgs`） |
| `pow(a, b)` | 2 | 幂运算 `a^b` |
| `clamp(x, lo, hi)` | 3 | 区间裁剪（`lo > hi` 返回 `InvalidArgs`） |
| `sign(x)` | 1 | 符号函数（`-1` / `0` / `1`） |
| `avg(a, b)` | 2 | 算术平均值 `(a + b) / 2` |
| `if(cond, a, b)` | 3 | 函数式条件分支 |

## 导出 API / Exported API

- `ExprError`: `None` / `SyntaxError` / `UnexpectedToken` / `DivisionByZero` / `UnknownVariable` / `UnknownFunction` / `InvalidArgs` / `MaxDepthExceeded`
- `EvalResult`: `{ ok: bool, value: f64, err: ExprError, pos: usize }`
- `ExprEnv`: `init()`、`set(*_, name, value)`、`get(name): f64?`、`has(name): bool`、`clear(*_)`、`len(): usize`
- `evalExpr(source: string): EvalResult`
- `evalExprWith(source: string, env: ExprEnv): EvalResult`
- `evalBool(source: string, env: ExprEnv): bool`

## 使用示例 / Examples

```cp
import { ExprEnv, ExprError, evalExpr, evalExprWith, evalBool } from "@cap/expr";

fn demo() {
    // 1. 直接求值（支持优先级、右结合幂运算、常量与内置函数）
    const r1 = evalExpr("(2 + 3) * 4 - 2 ^ 3 ^ 2");
    // r1.ok == true, r1.value == -492.0

    // 2. 变量绑定与动态公式计算
    let env = ExprEnv.init();
    env.set("price", 120.0);
    env.set("qty", 3.0);
    env.set("discount", 0.15);

    const total = evalExprWith("round(price * qty * (1 - discount))", env);
    // total.ok == true, total.value == 306.0

    // 3. 规则判定与布尔求值
    const is_vip_order = evalBool("price * qty >= 300 && discount <= 0.2", env);
    // is_vip_order == true

    // 4. 错误码安全捕获
    const bad = evalExpr("10 / (5 - 5)");
    // bad.ok == false, bad.err == ExprError.DivisionByZero
    _ = r1;
    _ = total;
    _ = is_vip_order;
    _ = bad;
}
```
