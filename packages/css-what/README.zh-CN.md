[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# css-what

Zero-allocation CSS selector tokenizer, parser, and serializer (Cheerio / css-select core) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-what` ([https://github.com/fb55/css-what](https://github.com/fb55/css-what))

| 元信息 | 值 |
| --- | --- |
| 包名 | `css-what` |
| 版本 | `1.0.0` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add css-what
```

```cp
import {
    CssTokenKind,
    AttrAction,
    CssSelectorToken,
    CwhWriter,
    cwhIsIdentStart,
    cwhIsIdentChar,
    cwhSkipSpace,
    cwhScanIdent,
} from "css-what";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CssTokenKind` | `Tag, Universal, Id, Class, Attribute, Pseudo, PseudoElement, Descendant ...` |
| `enum` | `AttrAction` | `Exists, Equals, Start, End, Any, Not, Element, Hyphen` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssSelectorToken` | `class` | 字段: `kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool` |
| `CwhWriter` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `writeByte, putStr` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cwhIsIdentStart` | `(c: u8): bool` | 调用 `cwhIsIdentStart` 并返回 `bool`。 |
| `cwhIsIdentChar` | `(c: u8): bool` | 调用 `cwhIsIdentChar` 并返回 `bool`。 |
| `cwhSkipSpace` | `(s: string, start: usize): usize` | 调用 `cwhSkipSpace` 并返回 `usize`。 |
| `cwhScanIdent` | `(s: string, start: usize): usize` | 调用 `cwhScanIdent` 并返回 `usize`。 |
| `cwhIsCombinatorChar` | `(c: u8): bool` | 调用 `cwhIsCombinatorChar` 并返回 `bool`。 |
| `cwhToken` | `(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken` | 调用 `cwhToken` 并返回 `CssSelectorToken`。 |
| `parseCssSelector` | `(selector: string, out: []CssSelectorToken): i64` | 通过 `parseCssSelector` 解析输入数据，返回 `i64`。 |
| `stringifyCssSelector` | `(tokens: &[]CssSelectorToken, out: []u8): usize` | 通过 `stringifyCssSelector` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseCssSelector } from "css-what";

pub fn main() {
    let result = parseCssSelector("example", /* out: []CssSelectorToken */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
