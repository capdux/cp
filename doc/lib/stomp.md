# `cap:stomp` — STOMP 1.2 协议帧编解码与增量流式解析库

> **模块路径**: `lib/stomp/`  
> **导入协议**: `import { StompCommand, StompHeader, StompFrame, StompBuilder, StompStreamParser, StompError, encodeFrame, escapeHeaderValue, unescapeHeaderValue } from "cap:stomp";`  
> **动态挂载**: `import cap from "cap";` -> `cap.stomp.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:stomp` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix` 或 `@import("bun")`）的 STOMP 1.2（Simple Text Oriented Messaging Protocol）协议帧编解码器与增量流式解析器：

1. **STOMP 1.2 头部值转义 (`escapeHeaderValue` / `unescapeHeaderValue`)**：
   - 完整实现 STOMP 1.2 转义映射：`\r` <-> `\r`、`\n` <-> `\n`、`:` <-> `\c`、`\` <-> `\\`。
   - 对非法反斜杠转义（如 `\x` 或末尾孤立 `\`）严格返回 `StompError.InvalidEscape`。
   - 遵循 STOMP 1.2 规范：`CONNECT` 与 `CONNECTED` 帧为保持 STOMP 1.0 兼容性**不执行**头部值转义与反转义。
2. **帧编码与构造 (`encodeFrame` / `StompBuilder`)**：
   - 校验头部名称非空且不含 `:`、`\r`、`\n`（否则返回 `StompError.InvalidHeader`）。
   - 携带非空 `body` 且未显式设置 `content-length` 时自动注入 `content-length:<len>`，并以 `\x00` 结束帧。
3. **增量流式解析 (`StompStreamParser`)**：
   - 支持跨任意字节分片（半包、黏包、1 字节切分）的确定性状态机拆帧。
   - 自动跳过帧间心跳 EOL（`\n` / `\r\n`），同时兼容 `\r\n` 与 `\n` 行尾。
   - 优先依据首个 `content-length` 读取二进制载荷（支持内嵌 `\x00`），并校验紧随其后的 `\x00` 帧终止符（缺失则返回 `StompError.TruncatedFrame`）。
   - 无 `content-length` 时扫描至 `\x00`；内置 `max_body_bytes` 上限防护 `StompError.BodyTooLarge`。
   - `StompFrame.header(name)` 遵循 STOMP 1.2 重复头规则，仅返回首个匹配头的值。

---

## 二、 核心 API 速查表 (`lib/stomp/stomp.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `StompError` | `InvalidCommand`, `InvalidHeader`, `InvalidEscape`, `InvalidContentLength`, `BodyTooLarge`, `TruncatedFrame`, `OutOfMemory` |
| `StompCommand` | `Connect`, `Stomp`, `Send`, `Subscribe`, `Unsubscribe`, `Ack`, `Nack`, `Begin`, `Commit`, `Abort`, `Disconnect`, `Connected`, `Message`, `Receipt`, `Error`；含 `toString()`、`parse(s)`、`skipsHeaderEscaping()` |
| `StompHeader` | `{ name: []const u8, value: []const u8 }` |
| `StompFrame` | `{ command: StompCommand, headers: []StompHeader, body: []const u8 }`；含 `deinit(allocator)`、`header(name)`、`contentType()`、`subscription()`、`messageId()` |
| `escapeHeaderValue(allocator, raw)` | 转义头部值（`\r`, `\n`, `:`, `\`），返回堆分配切片 |
| `unescapeHeaderValue(allocator, escaped)` | 反转义头部值，非法转义返回 `StompError.InvalidEscape` |
| `encodeFrame(allocator, frame)` | 编码 `StompFrame` 为 STOMP 1.2 线缆字节流（`\x00` 结尾） |
| `StompBuilder` | `init(allocator, command)`、`deinit()`、`addHeader(name, value)`、`setBody(body)`、`build()` |
| `StompStreamParser` | `init(allocator, max_body_bytes)`、`deinit()`、`feed(chunk)`、`nextFrame()` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { StompCommand, StompBuilder, StompStreamParser } from "cap:stomp";

fn demo(alloc: std.mem.Allocator): !void {
    let builder = StompBuilder.init(alloc, StompCommand.Send);
    defer builder.deinit();
    builder.addHeader("destination", "/queue/events:v1")?;
    builder.addHeader("content-type", "application/octet-stream")?;
    builder.setBody("hi\x00bin")?;

    const wire = builder.build()?;
    defer alloc.free(wire);

    let parser = StompStreamParser.init(alloc, 64 * 1024);
    defer parser.deinit();
    parser.feed(wire)?;

    if (frame := parser.nextFrame()) {
        defer frame.deinit(alloc);
        // frame.command == StompCommand.Send
        // frame.header("destination").? == "/queue/events:v1"
        // frame.body == "hi\x00bin"
    }
}
```
