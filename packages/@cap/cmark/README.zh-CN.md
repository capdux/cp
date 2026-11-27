[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/cmark

CommonMark 0.31.1 official C reference Markdown to HTML/XML/LaTeX renderer for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/cmark` |
| 版本 | `0.31.1` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/cmark
```

```cp
import {
    CmarkError,
    HtmlString,
    cmark_markdown_to_html,
    cmark_version,
    cmark_version_string,
    free,
    versionString,
    versionNumber,
} from "@cap/cmark";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `CmarkError` | `RenderFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HtmlString` | `class` | 字段: `raw: [*:0]u8!?, len: usize` · 方法: `slice, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmark_markdown_to_html` | `(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?` | 调用 `cmark_markdown_to_html` 并返回 `[*:0]u8!?`。 |
| `cmark_version` | `(): i32` | 调用 `cmark_version` 并返回 `i32`。 |
| `cmark_version_string` | `(): [*:0]&u8!?` | 调用 `cmark_version_string` 并返回 `[*:0]&u8!?`。 |
| `free` | `(ptr: *auto!?)` | 释放模块分配的底层资源。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |
| `versionNumber` | `(): i32` | 调用 `versionNumber` 并返回 `i32`。 |
| `markdownToHtml` | `(md: string, opts: i32): !HtmlString` | 调用 `markdownToHtml` 并返回 `!HtmlString`。 |

## 最小调用示例

```cp
import { markdownToHtml } from "@cap/cmark";

pub fn main() {
    let result = markdownToHtml("example", 0) catch return;
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
