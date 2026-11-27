[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# css-select

Zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matcher (clean-room port of css-select + css-what)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:css-select` ([https://github.com/fb55/css-select](https://github.com/fb55/css-select))

| 元信息 | 值 |
| --- | --- |
| 包名 | `css-select` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add css-select
```

```cp
import {
    Specificity,
    CssElement,
    csssIsCombinator,
    csssIsIdentStart,
    csssIsIdentChar,
    csssEqMaybeCi,
    csssSkipIdent,
    csssFindBracketEnd,
} from "css-select";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Specificity` | `class` | 字段: `ids: u32, classes: u32, tags: u32` · 方法: `new, cmp, toScore` |
| `CssElement` | `class` | 字段: `tag: string, id: string, class_list: string, attr_name: string, attr_val: string` · 方法: `new` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `csssIsCombinator` | `(c: u8): bool` | 调用 `csssIsCombinator` 并返回 `bool`。 |
| `csssIsIdentStart` | `(c: u8): bool` | 调用 `csssIsIdentStart` 并返回 `bool`。 |
| `csssIsIdentChar` | `(c: u8): bool` | 调用 `csssIsIdentChar` 并返回 `bool`。 |
| `csssEqMaybeCi` | `(a: string, b: string, ci: bool): bool` | 调用 `csssEqMaybeCi` 并返回 `bool`。 |
| `csssSkipIdent` | `(s: string, pos: usize): usize` | 调用 `csssSkipIdent` 并返回 `usize`。 |
| `csssFindBracketEnd` | `(s: string, pos: usize): usize` | 调用 `csssFindBracketEnd` 并返回 `usize`。 |
| `csssSkipParen` | `(s: string, pos: usize): usize` | 调用 `csssSkipParen` 并返回 `usize`。 |
| `csssHasToken` | `(list: string, target: string, ci: bool): bool` | 调用 `csssHasToken` 并返回 `bool`。 |
| `csssContains` | `(haystack: string, needle: string, ci: bool): bool` | 调用 `csssContains` 并返回 `bool`。 |
| `csssIsLegacyPseudoElement` | `(name: string): bool` | 调用 `csssIsLegacyPseudoElement` 并返回 `bool`。 |
| `csssComputeSingleSpecificity` | `(s: string, depth: u32): Specificity` | 调用 `csssComputeSingleSpecificity` 并返回 `Specificity`。 |
| `csssComputeListSpecificity` | `(selector: string, depth: u32): Specificity` | 调用 `csssComputeListSpecificity` 并返回 `Specificity`。 |
| `computeSpecificity` | `(selector: string): Specificity` | 调用 `computeSpecificity` 并返回 `Specificity`。 |
| `csssMatchAttr` | `(raw_inner: string, el: CssElement): bool` | 调用 `csssMatchAttr` 并返回 `bool`。 |
| `matchesCompoundSelector` | `(selector: string, el: CssElement): bool` | 调用 `matchesCompoundSelector` 并返回 `bool`。 |
| `matchesSelectorList` | `(selector_list: string, el: CssElement): bool` | 调用 `matchesSelectorList` 并返回 `bool`。 |
| `countSelectorParts` | `(selector: string): usize` | 调用 `countSelectorParts` 并返回 `usize`。 |

## 最小调用示例

```cp
import { csssEqMaybeCi } from "css-select";

pub fn main() {
    let result = csssEqMaybeCi("example", "example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
