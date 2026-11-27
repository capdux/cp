[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cexpr

Zero-allocation C constant expression parser and evaluator (full C operator precedence, hex/bin/oct/char literals) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cexpr` ([https://github.com/jethrogb/rust-cexpr](https://github.com/jethrogb/rust-cexpr))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cexpr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cexpr
```

```cp
import {
    EvalResult,
    Parser,
    skipWs,
    peek,
    peek2,
    absI64,
    signedDivI64,
    signedRemI64,
} from "cexpr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EvalResult` | `class` | 字段: `value: i64, ok: bool` |
| `Parser` | `class` | 字段: `pos: usize, ok: bool, depth: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `skipWs` | `(p: &mut Parser, src: string)` | 就地执行 `skipWs` 操作。 |
| `peek` | `(p: &mut Parser, src: string): u8` | 调用 `peek` 并返回 `u8`。 |
| `peek2` | `(p: &mut Parser, src: string): u8` | 调用 `peek2` 并返回 `u8`。 |
| `absI64` | `(x: i64): u64` | 调用 `absI64` 并返回 `u64`。 |
| `signedDivI64` | `(a: i64, b: i64): i64` | 调用 `signedDivI64` 并返回 `i64`。 |
| `signedRemI64` | `(a: i64, b: i64): i64` | 调用 `signedRemI64` 并返回 `i64`。 |
| `skipIntSuffix` | `(p: &mut Parser, src: string)` | 就地执行 `skipIntSuffix` 操作。 |
| `parseCharLit` | `(p: &mut Parser, src: string): i64` | 通过 `parseCharLit` 解析输入数据，返回 `i64`。 |
| `parseIntLit` | `(p: &mut Parser, src: string): i64` | 通过 `parseIntLit` 解析输入数据，返回 `i64`。 |
| `parsePrimary` | `(p: &mut Parser, src: string): i64` | 通过 `parsePrimary` 解析输入数据，返回 `i64`。 |
| `parseUnary` | `(p: &mut Parser, src: string): i64` | 通过 `parseUnary` 解析输入数据，返回 `i64`。 |
| `parseMul` | `(p: &mut Parser, src: string): i64` | 通过 `parseMul` 解析输入数据，返回 `i64`。 |
| `parseAdd` | `(p: &mut Parser, src: string): i64` | 通过 `parseAdd` 解析输入数据，返回 `i64`。 |
| `parseShift` | `(p: &mut Parser, src: string): i64` | 通过 `parseShift` 解析输入数据，返回 `i64`。 |
| `parseRel` | `(p: &mut Parser, src: string): i64` | 通过 `parseRel` 解析输入数据，返回 `i64`。 |
| `parseEq` | `(p: &mut Parser, src: string): i64` | 通过 `parseEq` 解析输入数据，返回 `i64`。 |
| `parseBitAnd` | `(p: &mut Parser, src: string): i64` | 通过 `parseBitAnd` 解析输入数据，返回 `i64`。 |
| `parseBitXor` | `(p: &mut Parser, src: string): i64` | 通过 `parseBitXor` 解析输入数据，返回 `i64`。 |
| `parseBitOr` | `(p: &mut Parser, src: string): i64` | 通过 `parseBitOr` 解析输入数据，返回 `i64`。 |
| `parseLogAnd` | `(p: &mut Parser, src: string): i64` | 通过 `parseLogAnd` 解析输入数据，返回 `i64`。 |
| `parseLogOr` | `(p: &mut Parser, src: string): i64` | 通过 `parseLogOr` 解析输入数据，返回 `i64`。 |
| `parseTernary` | `(p: &mut Parser, src: string): i64` | 通过 `parseTernary` 解析输入数据，返回 `i64`。 |
| `evalCExpr` | `(expr: string): EvalResult` | 调用 `evalCExpr` 并返回 `EvalResult`。 |

## 最小调用示例

```cp
import { parseCharLit } from "cexpr";

pub fn main() {
    let result = parseCharLit(/* p: &mut Parser */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
