[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/multipart

Official multipart/form-data parser and payload guard plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/multipart` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/multipart
```

```cp
import {
    MultipartConfig,
    MultipartPart,
    MultipartForm,
    mpAsciiLower,
    mpStartsWithIgnoreCase,
    mpEqlIgnoreCase,
    mpTrimSpaces,
    mpParseUsize,
} from "@web/multipart";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MultipartConfig` | `class` | 字段: `max_parts: usize, max_file_bytes: usize, max_total_bytes: usize, allowed_mime_prefix: string` |
| `MultipartPart` | `class` | 字段: `name: string, filename: string, content_type: string, data: string, is_file: bool` |
| `MultipartForm` | `class` | 字段: `parts: [16]MultipartPart, count: usize, total_bytes: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mpAsciiLower` | `(c: u8): u8` | 调用 `mpAsciiLower` 并返回 `u8`。 |
| `mpStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | 调用 `mpStartsWithIgnoreCase` 并返回 `bool`。 |
| `mpEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `mpEqlIgnoreCase` 并返回 `bool`。 |
| `mpTrimSpaces` | `(s: string): string` | 调用 `mpTrimSpaces` 并返回 `string`。 |
| `mpParseUsize` | `(s: string): usize?` | 调用 `mpParseUsize` 并返回 `usize?`。 |
| `mpMatchAt` | `(haystack: string, pos: usize, needle: string): bool` | 调用 `mpMatchAt` 并返回 `bool`。 |
| `mpFindBoundary` | `(body: string, start_pos: usize, boundary: string): usize?` | 调用 `mpFindBoundary` 并返回 `usize?`。 |
| `mpFindHeaderEnd` | `(part: string, body_start_out: &mut usize): usize?` | 调用 `mpFindHeaderEnd` 并返回 `usize?`。 |
| `mpExtractParam` | `(header_val: string, key: string): string?` | 调用 `mpExtractParam` 并返回 `string?`。 |
| `parseMultipartBody` | `(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32` | 通过 `parseMultipartBody` 解析输入数据，返回 `i32`。 |
| `multipartGuardHook` | `(ctx: &mut Context): bool` | 调用 `multipartGuardHook` 并返回 `bool`。 |
| `createMultipartPlugin` | `(cfg: MultipartConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerMultipart` | `(app: &mut App, cfg: MultipartConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { parseMultipartBody } from "@web/multipart";

pub fn main() {
    let result = parseMultipartBody("example", "example", /* cfg: MultipartConfig */, /* out_form: &mut MultipartForm */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
