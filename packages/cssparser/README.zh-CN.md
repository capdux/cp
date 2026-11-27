[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cssparser

Zero-heap CSS Syntax Level 3 tokenizer and lexer in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `cssparser` |
| 版本 | `0.34.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cssparser
```

```cp
import {
    CssTokKind,
    CssToken,
    CssLexer,
    csspIsIdentStart,
    csspIsIdentChar,
    csspStartsIdent,
    csspStartsNumber,
    csspScanIdent,
} from "cssparser";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CssTokKind` | `Ident, AtKeyword, Hash, String, Number, Percentage, Dimension, Function ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssToken` | `class` | 字段: `kind: CssTokKind, text: string, unit: string, start: usize, end: usize` |
| `CssLexer` | `class` | 字段: `src: string, pos: usize` · 方法: `new, nextToken` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `csspIsIdentStart` | `(c: u8): bool` | 调用 `csspIsIdentStart` 并返回 `bool`。 |
| `csspIsIdentChar` | `(c: u8): bool` | 调用 `csspIsIdentChar` 并返回 `bool`。 |
| `csspStartsIdent` | `(s: string, pos: usize): bool` | 调用 `csspStartsIdent` 并返回 `bool`。 |
| `csspStartsNumber` | `(s: string, pos: usize): bool` | 调用 `csspStartsNumber` 并返回 `bool`。 |
| `csspScanIdent` | `(s: string, start: usize): usize` | 调用 `csspScanIdent` 并返回 `usize`。 |
| `csspScanNumber` | `(s: string, start: usize): usize` | 调用 `csspScanNumber` 并返回 `usize`。 |
| `csspSkipWsAndComments` | `(s: string, start: usize): usize` | 调用 `csspSkipWsAndComments` 并返回 `usize`。 |

## 最小调用示例

```cp
import { csspStartsIdent } from "cssparser";

pub fn main() {
    let result = csspStartsIdent("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
