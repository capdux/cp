# `cap:multipart` — RFC 7578 `multipart/form-data` 解析、边界提取与构建库

> **模块路径**: `lib/multipart/`  
> **导入协议**: `import { MultipartBuilder, MultipartDocument, MultipartPart, MultipartError, extractBoundary, parse, parseWithLimits } from "cap:multipart";`  
> **动态挂载**: `import cap from "cap";` -> `cap.multipart.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:multipart` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix` 或 `@import("bun")`）的 RFC 7578 `multipart/form-data` 解析与构建库：

1. **边界提取与 RFC 2046 校验 (`extractBoundary`)**：
   - 大小写不敏感匹配 `multipart/form-data` 媒体类型与 `boundary` 参数，支持分号多参数与带引号 `boundary="..."`。
   - 严格校验边界长度 `1..70` 及合法字符集（拒绝控制字符、未闭合引号、空边界与非 `multipart/form-data` 类型）。
2. **二进制安全解析 (`parse` / `parseWithLimits`)**：
   - 兼容 `\r\n` 与 `\n` 换行符，支持前置 preamble 与尾随 epilogue。
   - 完整保留二进制载荷中的内嵌 `\x00` 与 `\r\n`。
   - 解析 `Content-Disposition` 中的 `name` 与 `filename`（支持 `\"` / `\\` 转义还原）以及 `Content-Type`（缺失时默认 `"text/plain"`）。
   - 严格校验结束边界 `--boundary--`（缺失返回 `MissingEndBoundary`），并提供 `max_parts` 与 `max_part_bytes` 资源耗尽防护。
3. **安全增量构建 (`MultipartBuilder`)**：
   - 支持 `addField` 与 `addFile`，自动拦截字段名、文件名或 `Content-Type` 中的 CR/LF/NUL 头注入（返回 `InvalidHeaderValue`）。
   - 自动生成规范 `Content-Type` 请求头与完整 `--boundary--\r\n` 载荷。

---

## 二、 核心 API 速查表 (`lib/multipart/multipart.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `MultipartError` | `InvalidBoundary`, `InvalidContentType`, `MalformedPartHeader`, `MissingEndBoundary`, `TooManyParts`, `PartTooLarge`, `InvalidHeaderValue`, `OutOfMemory` |
| `MultipartPart` | `{ name: []const u8, filename: ([]const u8)?, content_type: []const u8, data: []const u8 }` |
| `MultipartDocument.findField(name)` | 返回首个匹配 `name` 的 `MultipartPart?` |
| `MultipartDocument.findAll(allocator, name)` | 返回所有匹配 `name` 的 `[]MultipartPart`（调用方负责 `allocator.free` 切片） |
| `MultipartDocument.deinit(allocator)` | 释放文档及其拥有的所有分片与字符串内存 |
| `extractBoundary(content_type_header)` | 从 `Content-Type` 头提取并校验 `boundary` 字符串 |
| `parse(allocator, boundary, body)` | 使用默认限制解析 `multipart/form-data` 请求体 |
| `parseWithLimits(allocator, boundary, body, max_parts, max_part_bytes)` | 带分片数量与单分片体积上限的解析 |
| `MultipartBuilder.init(allocator, boundary)` | 使用已校验的 `boundary` 初始化构建器 |
| `MultipartBuilder.deinit()` | 释放构建器内部缓冲区 |
| `MultipartBuilder.addField(name, value)` | 添加文本表单字段（含 CR/LF 注入拦截） |
| `MultipartBuilder.addFile(name, filename, content_type, data)` | 添加文件分片（含 CR/LF 注入拦截） |
| `MultipartBuilder.contentTypeHeader(allocator)` | 生成 `"multipart/form-data; boundary=..."` 字符串 |
| `MultipartBuilder.finish(allocator)` | 生成完整 `multipart/form-data` 请求体（以 `--boundary--\r\n` 结尾） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { MultipartBuilder, extractBoundary, parse } from "cap:multipart";

fn demo(alloc: std.mem.Allocator): !void {
    let builder = MultipartBuilder.init(alloc, "----CPBoundary123")?;
    defer builder.deinit();

    builder.addField("username", "alice")?;
    builder.addFile("avatar", "pic.png", "image/png", "\x89PNG\r\n\x00data")?;

    const ct = builder.contentTypeHeader(alloc)?;
    defer alloc.free(ct);
    const body = builder.finish(alloc)?;
    defer alloc.free(body);

    const boundary = extractBoundary(ct)?;
    let doc = parse(alloc, boundary, body)?;
    defer doc.deinit(alloc);

    if (user := doc.findField("username")) {
        * = user.data;
    }
}
```
