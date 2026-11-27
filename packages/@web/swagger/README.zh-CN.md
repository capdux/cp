[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/swagger

Official OpenAPI 3.1.0 & interactive Scalar/Swagger UI documentation plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/swagger` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/swagger
```

```cp
import {
    SwaggerConfig,
    swAsciiLower,
    swAsciiEqlIgnoreCase,
    swWriteByte,
    swWriteRaw,
    swWriteHtmlEscaped,
    renderDocsHtml,
    refreshSpec,
} from "@web/swagger";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SwaggerConfig` | `class` | 字段: `title: string, version: string, spec_path: string, docs_path: string, ui_theme: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `swAsciiLower` | `(c: u8): u8` | 调用 `swAsciiLower` 并返回 `u8`。 |
| `swAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `swAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `swWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `swWriteByte` 并返回 `bool`。 |
| `swWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `swWriteRaw` 并返回 `bool`。 |
| `swWriteHtmlEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `swWriteHtmlEscaped` 并返回 `bool`。 |
| `renderDocsHtml` | `(cfg: SwaggerConfig, out_buf: []u8): usize?` | 调用 `renderDocsHtml` 并返回 `usize?`。 |
| `refreshSpec` | `(app: &mut App): usize?` | 调用 `refreshSpec` 并返回 `usize?`。 |
| `cachedSpec` | `(): string` | 调用 `cachedSpec` 并返回 `string`。 |
| `cachedDocsHtml` | `(): string` | 调用 `cachedDocsHtml` 并返回 `string`。 |
| `swOpenApiSpecHandler` | `(ctx: &mut Context): Response` | 调用 `swOpenApiSpecHandler` 并返回 `Response`。 |
| `swDocsUiHandler` | `(ctx: &mut Context): Response` | 调用 `swDocsUiHandler` 并返回 `Response`。 |
| `registerSwagger` | `(app: &mut App, cfg: SwaggerConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { swAsciiEqlIgnoreCase } from "@web/swagger";

pub fn main() {
    let result = swAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
