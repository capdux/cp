[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/static

Official zero-GC static file serving plugin for cap:web (path traversal guard, extension + magic-number MIME detection, FNV-1a ETag & 304 Not Modified)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/static` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/static
```

```cp
import {
    StaticConfig,
    stAsciiLower,
    stAsciiEqlIgnoreCase,
    stTrimAsciiSpaces,
    stHexVal,
    stHexDigitLower,
    stWriteByte,
    stWriteRaw,
} from "@web/static";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StaticConfig` | `class` | 字段: `root_dir: string, url_prefix: string, index_file: string, cache_control: string, enable_etag: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `stAsciiLower` | `(c: u8): u8` | 调用 `stAsciiLower` 并返回 `u8`。 |
| `stAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `stAsciiEqlIgnoreCase` 并返回 `bool`。 |
| `stTrimAsciiSpaces` | `(s: string): string` | 调用 `stTrimAsciiSpaces` 并返回 `string`。 |
| `stHexVal` | `(c: u8): u8?` | 调用 `stHexVal` 并返回 `u8?`。 |
| `stHexDigitLower` | `(n: u8): u8` | 调用 `stHexDigitLower` 并返回 `u8`。 |
| `stWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `stWriteByte` 并返回 `bool`。 |
| `stWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `stWriteRaw` 并返回 `bool`。 |
| `sanitizeRelPath` | `(raw: string, out_buf: []u8): usize?` | 调用 `sanitizeRelPath` 并返回 `usize?`。 |
| `detectMimeType` | `(path: string, content: string): string` | 调用 `detectMimeType` 并返回 `string`。 |
| `computeStrongEtag` | `(content: string, out_buf: []u8): usize?` | 调用 `computeStrongEtag` 并返回 `usize?`。 |
| `etagMatches` | `(if_none_match: string, etag: string): bool` | 调用 `etagMatches` 并返回 `bool`。 |
| `stReadRegularFileInto` | `(full_path: string, out_buf: []u8): usize?` | 调用 `stReadRegularFileInto` 并返回 `usize?`。 |
| `stJoinFsPath` | `(root_dir: string, rel_path: string, out_buf: []u8): usize?` | 调用 `stJoinFsPath` 并返回 `usize?`。 |
| `stServeWithEtagBuf` | `(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8, etag_buf: []u8): Response` | 调用 `stServeWithEtagBuf` 并返回 `Response`。 |
| `serveStaticRequest` | `(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8): Response` | 调用 `serveStaticRequest` 并返回 `Response`。 |
| `serveFileFromDir` | `(root_dir: string, rel_path: string, out_buf: []u8): Response` | 调用 `serveFileFromDir` 并返回 `Response`。 |
| `serveStaticIntoArena` | `(ctx: &mut Context, cfg: StaticConfig, rel_path: string): Response` | 调用 `serveStaticIntoArena` 并返回 `Response`。 |
| `stRouteHandler` | `(ctx: &mut Context): Response` | 调用 `stRouteHandler` 并返回 `Response`。 |
| `registerStatic` | `(app: &mut App, cfg: StaticConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { stAsciiEqlIgnoreCase } from "@web/static";

pub fn main() {
    let result = stAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
