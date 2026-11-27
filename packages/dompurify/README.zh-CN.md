[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dompurify

Zero-allocation HTML sanitizer stripping blocked tags, event attributes, and unsafe URI schemes (npm DOMPurify parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`dompurify`](https://github.com/cure53/DOMPurify)（`Apache-2.0`）（包名 `dompurify`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `dompurify` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add dompurify
```

```cp
import {
    SanitizeHtmlOptions,
    defaultSanitizeOptions,
    dmpIsNameChar,
    dmpHasPrefixCI,
    dmpWriteByte,
    dmpIsBlockedContentTag,
    dmpIsBlockedVoidTag,
    dmpIsAllowedTag,
} from "dompurify";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SanitizeHtmlOptions` | `class` | 字段: `allow_links: bool, allow_images: bool, strip_comments: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultSanitizeOptions` | `(): SanitizeHtmlOptions` | 调用 `defaultSanitizeOptions` 并返回 `SanitizeHtmlOptions`。 |
| `dmpIsNameChar` | `(c: u8): bool` | 调用 `dmpIsNameChar` 并返回 `bool`。 |
| `dmpHasPrefixCI` | `(s: string, prefix: string): bool` | 调用 `dmpHasPrefixCI` 并返回 `bool`。 |
| `dmpWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `dmpWriteByte` 并返回 `usize`。 |
| `dmpIsBlockedContentTag` | `(tag: string): bool` | 调用 `dmpIsBlockedContentTag` 并返回 `bool`。 |
| `dmpIsBlockedVoidTag` | `(tag: string): bool` | 调用 `dmpIsBlockedVoidTag` 并返回 `bool`。 |
| `dmpIsAllowedTag` | `(tag: string, opts: SanitizeHtmlOptions): bool` | 调用 `dmpIsAllowedTag` 并返回 `bool`。 |
| `dmpIsEventAttr` | `(attr: string): bool` | 调用 `dmpIsEventAttr` 并返回 `bool`。 |
| `dmpIsSafeUrl` | `(url: string): bool` | 调用 `dmpIsSafeUrl` 并返回 `bool`。 |
| `dmpIsSafeAttr` | `(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool` | 调用 `dmpIsSafeAttr` 并返回 `bool`。 |
| `dmpFindTagClose` | `(html: string, start: usize): usize` | 调用 `dmpFindTagClose` 并返回 `usize`。 |
| `dmpFindClosingTagEnd` | `(html: string, start: usize, tag: string): usize` | 调用 `dmpFindClosingTagEnd` 并返回 `usize`。 |
| `sanitizeHtml` | `(html: string, opts: SanitizeHtmlOptions, out: []u8): usize` | 调用 `sanitizeHtml` 并返回 `usize`。 |
| `isSafeHtml` | `(html: string): bool` | 执行 `isSafeHtml` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isSafeHtml } from "dompurify";

pub fn main() {
    let result = isSafeHtml("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
