[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cssstyle

Zero-allocation CSS inline style declaration parser, query, mutator, and serializer (npm cssstyle parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`cssstyle`](https://github.com/jsdom/cssstyle)（`MIT`）（包名 `cssstyle`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `cssstyle` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cssstyle
```

```cp
import {
    CssProp,
    CstParsedValue,
    CssStyleDecl,
    cstSkipSpaceAndComments,
    cstStripTrailingComments,
    cstExtractImportant,
} from "cssstyle";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssProp` | `class` | 字段: `name: string, value: string, important: bool` |
| `CstParsedValue` | `class` | 字段: `value: string, important: bool` |
| `CssStyleDecl` | `class` | 字段: `props: [64]CssProp, count: usize` · 方法: `new, len, parse, getPropertyValue, getPropertyPriority, setProperty, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cstSkipSpaceAndComments` | `(s: string, pos: usize): usize` | 调用 `cstSkipSpaceAndComments` 并返回 `usize`。 |
| `cstStripTrailingComments` | `(s: string): string` | 调用 `cstStripTrailingComments` 并返回 `string`。 |
| `cstExtractImportant` | `(raw: string): CstParsedValue` | 调用 `cstExtractImportant` 并返回 `CstParsedValue`。 |

## 最小调用示例

```cp
import { cstSkipSpaceAndComments } from "cssstyle";

pub fn main() {
    let result = cstSkipSpaceAndComments("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
