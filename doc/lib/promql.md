# `cap:promql` — PromQL 词法与 AST 递归下降解析器

## 1. 概述

`cap:promql` 是纯 CP 实现的工业级 PromQL（Prometheus Query Language）词法分析器与递归下降（含运算符优先级爬升）AST 解析器标准库，支持即时向量/区间向量选择器、标签匹配器（`=`、`!=`、`=~`、`!~`）、`offset` 修饰符、聚合操作与分组修饰符（`by` / `without`）、函数调用、二元算术/比较/逻辑运算符及向量匹配修饰符（`on` / `ignoring`、`group_left` / `group_right`），并支持通过 `cap.promql` 动态挂载。

## 2. 核心类型

- **`PromqlError`**：解析与词法错误集合（`UnexpectedToken`、`UnexpectedEof`、`InvalidNumber`、`InvalidDuration`、`InvalidString`、`InvalidMatcher`、`InvalidModifier`、`UnmatchedParen`、`UnmatchedBrace`、`UnmatchedBracket`、`EmptyExpression`、`OutOfMemory`）。
- **`MatchOp`**：标签匹配操作符枚举（`Eq`, `Neq`, `Regex`, `NotRegex`）。
- **`LabelMatcher`**：标签匹配器结构体（`name`, `op`, `value`）。
- **`VectorSelector` / `MatrixSelector`**：即时向量与区间向量选择器结构体（含 `matchers`、`offset_ms`、`range_ms`）。
- **`AggregationOp` / `AggregationExpr`**：聚合操作符（`Sum`, `Avg`, `Min`, `Max`, `Count`, `Topk`, `Bottomk`）及聚合表达式（支持 `by`/`without` 与 `topk`/`bottomk` 参数）。
- **`BinaryOp` / `VectorMatching` / `BinaryExpr`**：二元运算符、`on`/`ignoring` 与 `group_left`/`group_right` 向量匹配修饰符、`bool` 修饰符。
- **`Expr`**：PromQL AST 标签联合体（`NumberLiteral`、`StringLiteral`、`Vector`、`Matrix`、`Aggregation`、`Binary`、`Unary`、`Call`、`Paren`），提供 `deinit(alloc)` 递归释放全部堆内存。

## 3. API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` | `parse(alloc: std.mem.Allocator, input: []const u8) -> !Expr` | 将 PromQL 查询字符串解析为 AST 根节点 `Expr` |
| `parseDuration` | `parseDuration(s: []const u8) -> !i64` | 解析 PromQL 时长字面量（支持 `ms`/`s`/`m`/`h`/`d`/`w`/`y` 及复合时长），返回毫秒数 |
| `Expr.deinit` | `expr.deinit(alloc: std.mem.Allocator)` | 递归释放 AST 节点及关联切片内存 |

## 4. 示例

```typescript
import std from "std";
import { parse } from "cap:promql";
import cap from "cap";

fn main() -> ! {
    const alloc = std.heap.page_allocator;
    let ast = parse(alloc, "rate(http_requests_total{code=~\"5..\"}[5m])")?;
    defer ast.deinit(alloc);

    let dyn_ast = cap.promql.parse(alloc, "sum by (le) (lat_bucket[5m])")?;
    defer dyn_ast.deinit(alloc);
}
```
