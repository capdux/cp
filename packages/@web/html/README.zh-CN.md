[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/html

Official HTML SSR, zero-heap XSS escaping, and HTMX helpers plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/html` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/html
```

```cp
import {
    HtmlConfig,
    HtmlBuilder,
    htmAsciiLower,
    htmAsciiEqlIgnoreCase,
    htmWriteByte,
    htmWriteRaw,
    htmEscapeInto,
    htmlEscape,
} from "@web/html";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HtmlConfig` | `class` | 字段: `default_charset: string` |
| `HtmlBuilder` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `init, raw, text, tag, tagRaw, slice, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `htmAsciiLower` | `(c: u8): u8` | 调用 `htmAsciiLower` 并返回 `u8`。 |
| `htmAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `htmAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `htmWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `htmWriteByte` 并返回 `bool`。 |
| `htmWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `htmWriteRaw` 并返回 `bool`。 |
| `htmEscapeInto` | `(out_buf: []u8, pos: &mut usize, input: string): bool` | 调用 `htmEscapeInto` 并返回 `bool`。 |
| `htmlEscape` | `(input: string, out_buf: []u8): usize?` | 调用 `htmlEscape` 并返回 `usize?`。 |
| `htmlIsHtmx` | `(ctx: &Context): bool` | 调用 `htmlIsHtmx` 并返回 `bool`。 |
| `htmlHxRedirect` | `(url: string): Response` | 调用 `htmlHxRedirect` 并返回 `Response`。 |
| `htmlHxTrigger` | `(body_html: string, event_name: string): Response` | 调用 `htmlHxTrigger` 并返回 `Response`。 |
| `htmAfterHook` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `htmAfterHook` 操作。 |
| `registerHtml` | `(app: &mut App, cfg: HtmlConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { htmAsciiEqlIgnoreCase } from "@web/html";

pub fn main() {
    let result = htmAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
