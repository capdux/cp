[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# spdx-expression-parse

Zero-allocation SPDX license expression recursive-descent AST parser and evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:spdx-expression-parse` ([https://github.com/jslicense/spdx-expression-parse.js](https://github.com/jslicense/spdx-expression-parse.js))

| 元信息 | 值 |
| --- | --- |
| 包名 | `spdx-expression-parse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add spdx-expression-parse
```

```cp
import {
    SpdxAstNode,
    SpdxAst,
    SpdxpState,
    spdxpEmptyNode,
    spdxpEmptyAst,
    spdxpIsIdChar,
    spdxpIsKeyword,
    spdxpIsAllIdChars,
} from "spdx-expression-parse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SpdxAstNode` | `class` | 字段: `kind: u8, license: string, exception: string, plus: bool, left: i32, right: i32` |
| `SpdxAst` | `class` | 字段: `nodes: [32]SpdxAstNode, root: i32, node_count: usize, valid: bool` |
| `SpdxpState` | `class` | 字段: `src: string, pos: usize, depth: usize, failed: bool, ast: SpdxAst` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `spdxpEmptyNode` | `(): SpdxAstNode` | 调用 `spdxpEmptyNode` 并返回 `SpdxAstNode`。 |
| `spdxpEmptyAst` | `(): SpdxAst` | 调用 `spdxpEmptyAst` 并返回 `SpdxAst`。 |
| `spdxpIsIdChar` | `(ch: u8): bool` | 调用 `spdxpIsIdChar` 并返回 `bool`。 |
| `spdxpIsKeyword` | `(tok: string): bool` | 调用 `spdxpIsKeyword` 并返回 `bool`。 |
| `spdxpIsAllIdChars` | `(s: string): bool` | 调用 `spdxpIsAllIdChars` 并返回 `bool`。 |
| `spdxpIsValidLicenseRef` | `(id: string): bool` | 调用 `spdxpIsValidLicenseRef` 并返回 `bool`。 |
| `spdxpIsValidLicenseId` | `(id: string): bool` | 调用 `spdxpIsValidLicenseId` 并返回 `bool`。 |
| `spdxpIsValidExceptionId` | `(exc: string): bool` | 调用 `spdxpIsValidExceptionId` 并返回 `bool`。 |
| `spdxpSkipSpaces` | `(st: &mut SpdxpState)` | 就地执行 `spdxpSkipSpaces` 操作。 |
| `spdxpAllocNode` | `(st: &mut SpdxpState, node: SpdxAstNode): i32` | 调用 `spdxpAllocNode` 并返回 `i32`。 |
| `spdxpConsumeKeyword` | `(st: &mut SpdxpState, kw: string): bool` | 调用 `spdxpConsumeKeyword` 并返回 `bool`。 |
| `spdxpParseSimple` | `(st: &mut SpdxpState): i32` | 调用 `spdxpParseSimple` 并返回 `i32`。 |
| `spdxpParsePrimary` | `(st: &mut SpdxpState): i32` | 调用 `spdxpParsePrimary` 并返回 `i32`。 |
| `spdxpParseAnd` | `(st: &mut SpdxpState): i32` | 调用 `spdxpParseAnd` 并返回 `i32`。 |
| `spdxpParseOr` | `(st: &mut SpdxpState): i32` | 调用 `spdxpParseOr` 并返回 `i32`。 |
| `parseSpdxExpression` | `(expr: string): SpdxAst` | 通过 `parseSpdxExpression` 解析输入数据，返回 `SpdxAst`。 |
| `isValidSpdxExpression` | `(expr: string): bool` | 执行 `isValidSpdxExpression` 条件判定，返回 `bool`。 |
| `spdxpMatchLicenseNode` | `(node: SpdxAstNode, allowed: string): bool` | 调用 `spdxpMatchLicenseNode` 并返回 `bool`。 |
| `spdxpEvalAst` | `(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool` | 调用 `spdxpEvalAst` 并返回 `bool`。 |
| `satisfiesSpdx` | `(ast: SpdxAst, allowed_license: string): bool` | 调用 `satisfiesSpdx` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseSpdxExpression } from "spdx-expression-parse";

pub fn main() {
    let result = parseSpdxExpression("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
