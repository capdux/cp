# `cap:sse` — W3C Server-Sent Events (`text/event-stream`) 增量解析与编码库

> **模块路径**: `lib/sse/`  
> **导入协议**: `import { SseParser, SseEncoder, SseEvent, SseError } from "cap:sse";`  
> **动态挂载**: `import cap from "cap";` -> `cap.sse.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:sse` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix` 或 `@import("bun")`）的 WHATWG/W3C Server-Sent Events (`text/event-stream`) 增量流式解析器与帧安全编码器：

1. **增量流式解析 (`SseParser`)**：
   - 自动剥离流首部可选 UTF-8 BOM（`\xEF\xBB\xBF`），支持跨 Chunk 单字节切分识别。
   - 完整兼容 `\r\n`、`\n`、`\r` 三种换行符及 `\r` 与 `\n` 跨 Chunk 拆分边界。
   - 严格遵循 W3C 冒号前导空格规则：仅剥离冒号后的**单个**前导 `' '`（`0x20`），保留后续多余空格。
   - 多行 `data:` 自动以 `\n` 拼接并去除末尾单个尾部 `\n`；空 `data` 缓冲区在遇到空行时不触发事件；未指定 `event:` 时默认为 `"message"`。
   - 忽略 `:comment` 注释行；`id:` 包含 `\x00` 时按规范忽略且不更新 `last_event_id`；`retry:` 仅全 ASCII 数字有效。
   - 内置 `max_event_bytes` 缓冲区上限，防止未终止大帧引起的资源耗尽（返回 `SseError.EventBufferOverflow`）。
2. **帧安全编码器 (`SseEncoder`)**：
   - `formatEvent` 自动将多行 `data` 按 `\n` 拆分为多行 `data: ...\n`，末尾输出 `\n` 空行分隔符。
   - 严格拦截 `event_type` / `id` / `comment` 中的 `\r` 与 `\n` 注入，返回 `SseError.InvalidEventData`。

---

## 二、 核心 API 速查表 (`lib/sse/sse.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `SseError` | `EventBufferOverflow`, `InvalidRetryValue`, `InvalidFieldName`, `InvalidEventData`, `OutOfMemory` |
| `SseEvent` | `{ event_type: []const u8, data: []const u8, id: ([]const u8)?, retry_ms: u64? }`；调用 `deinit(allocator)` 释放 |
| `SseParser.init(allocator, max_event_bytes)` | 初始化增量 SSE 解析器并设置单事件缓冲字节上限 |
| `SseParser.deinit()` | 释放内部行/事件缓冲区及所有未消费的 `SseEvent` |
| `SseParser.feed(chunk)` | 喂入任意长度字节分片，可能返回 `EventBufferOverflow` / `OutOfMemory` |
| `SseParser.nextEvent()` | 弹出下一个已就绪的 `SseEvent?`（调用方负责 `ev.deinit(allocator)`） |
| `SseParser.lastEventId()` | 获取当前最新有效的 `Last-Event-ID`（`[]const u8`） |
| `SseParser.retryMs()` | 获取当前最新有效的 `retry:` 重连间隔毫秒数（`u64?`） |
| `SseEncoder.formatEvent(allocator, event_type, data, id, retry_ms)` | 编码单条 SSE 事件帧（调用方负责 `allocator.free`） |
| `SseEncoder.formatComment(allocator, text)` | 编码 `: <text>\n` 注释行（调用方负责 `allocator.free`） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { SseParser, SseEncoder } from "cap:sse";

fn demo(alloc: std.mem.Allocator): !void {
    const frame = SseEncoder.formatEvent(alloc, "token", "hello\nworld", "evt-1", 3000)?;
    defer alloc.free(frame);

    let parser = SseParser.init(alloc, 64 * 1024);
    defer parser.deinit();

    parser.feed(frame)?;
    if (ev := parser.nextEvent()) {
        defer ev.deinit(alloc);
        // ev.event_type == "token", ev.data == "hello\nworld", ev.id == "evt-1", ev.retry_ms == 3000
    }
}
```
