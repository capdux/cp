[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# mathjs

Zero-allocation mathematical expression parser & evaluator (npm mathjs parity) composing libm with scoped variables, constants, 14 built-in functions, and strict error reporting for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mathjs` ([https://github.com/josdejong/mathjs](https://github.com/josdejong/mathjs))（原库名 `mathjs` → CP 包名 `mathjs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `mathjs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add mathjs
```

```cp
import {
    ScopeVar,
    ScopeLookup,
    EvalResult,
    MathScope,
    MjParser,
    mjStrEq,
    mjCopyBuf,
    mjIsSpace,
} from "mathjs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ScopeVar` | `class` | 字段: `name: [32]u8, name_len: usize, value: f64` |
| `ScopeLookup` | `class` | 字段: `found: bool, value: f64` |
| `EvalResult` | `class` | 字段: `ok: bool, value: f64, err_code: i32` |
| `MathScope` | `class` | 字段: `vars: [32]ScopeVar, count: usize` · 方法: `new, set, get` |
| `MjParser` | `class` | 字段: `src: string, pos: usize, depth: usize, scope: MathScope` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mjStrEq` | `(a: string, b: string): bool` | 调用 `mjStrEq` 并返回 `bool`。 |
| `mjCopyBuf` | `(dst: []u8, src: string): usize` | 调用 `mjCopyBuf` 并返回 `usize`。 |
| `mjIsSpace` | `(ch: u8): bool` | 调用 `mjIsSpace` 并返回 `bool`。 |
| `mjIsDigit` | `(ch: u8): bool` | 调用 `mjIsDigit` 并返回 `bool`。 |
| `mjIsIdentStart` | `(ch: u8): bool` | 调用 `mjIsIdentStart` 并返回 `bool`。 |
| `mjIsIdentChar` | `(ch: u8): bool` | 调用 `mjIsIdentChar` 并返回 `bool`。 |
| `mjOk` | `(v: f64): EvalResult` | 调用 `mjOk` 并返回 `EvalResult`。 |
| `mjErr` | `(code: i32): EvalResult` | 调用 `mjErr` 并返回 `EvalResult`。 |
| `mjSkipWs` | `(p: &mut MjParser)` | 就地执行 `mjSkipWs` 操作。 |
| `mjParseNumber` | `(p: &mut MjParser): EvalResult` | 调用 `mjParseNumber` 并返回 `EvalResult`。 |
| `mjEvalBuiltin` | `(name: string, args: &[]f64, argc: usize): EvalResult` | 调用 `mjEvalBuiltin` 并返回 `EvalResult`。 |
| `mjParsePrimary` | `(p: &mut MjParser): EvalResult` | 调用 `mjParsePrimary` 并返回 `EvalResult`。 |
| `mjParsePower` | `(p: &mut MjParser): EvalResult` | 调用 `mjParsePower` 并返回 `EvalResult`。 |
| `mjParseUnary` | `(p: &mut MjParser): EvalResult` | 调用 `mjParseUnary` 并返回 `EvalResult`。 |
| `mjParseMulDiv` | `(p: &mut MjParser): EvalResult` | 调用 `mjParseMulDiv` 并返回 `EvalResult`。 |
| `mjParseAddSub` | `(p: &mut MjParser): EvalResult` | 调用 `mjParseAddSub` 并返回 `EvalResult`。 |
| `evalExprWithScope` | `(expr: string, scope: MathScope): EvalResult` | 调用 `evalExprWithScope` 并返回 `EvalResult`。 |
| `evalExpr` | `(expr: string): EvalResult` | 调用 `evalExpr` 并返回 `EvalResult`。 |

## 最小调用示例

```cp
import { mjStrEq } from "mathjs";

pub fn main() {
    let result = mjStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
