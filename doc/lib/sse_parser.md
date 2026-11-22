# `cap:sse_parser` — WHATWG Server-Sent Events (`text/event-stream`) 解析与格式化库

> 参考来源：/data/dev/bun/src/http/HTTPRequestBody.cp | 吸收日期：2026-11-22

`cap:sse_parser` 提供纯 CP、零外部依赖、零堆分配的 `text/event-stream` 流式事件解析与格式化能力，适用于 LLM 流式增量输出（`data: {"delta":"..."}`）及实时服务端事件推送场景。

## 核心类型

### `SseEvent`

```cp
pub struct SseEvent {
    event_type: []const u8 = "message",
    data: []const u8 = "",
    id: []const u8 = "",
    has_id: bool = false,
    retry_ms: u64? = null,
}
```

### `SseParser`

```cp
pub struct SseParser {
    static fn init(src: []const u8): SseParser
    fn next(*_): SseEvent?
    fn lastEventId(): []const u8
}
```

严格遵循 WHATWG SSE 规范（§9.2.6）：
- 自动剥离可选 UTF-8 BOM，统一兼容 `\r\n`、`\n`、`\r` 行尾。
- 忽略以 `:` 开头的注释行（如 `: keep-alive`）。
- 冒号后最多剥离 **1 个**前导空格（保留后续空格）。
- 同一事件内的多行 `data:` 自动通过内部 4096 字节缓冲以 `\n` 拼接，仅在空行时派发含 `data:` 的事件。
- 忽略含 NUL（`\0`）的 `id:` 字段；`retry:` 仅接受纯 ASCII 数字。
- 通过 `lastEventId()` 持续追踪最新合法事件 ID。

## 核心函数

### `formatSseEvent`

```cp
pub fn formatSseEvent(ev: SseEvent, out: []u8): ([]const u8)?
```

- 将 `SseEvent` 序列化为标准 `text/event-stream` 帧，自动将多行 `data` 拆分为多条 `data: ...` 行并以 `\n\n` 结尾；缓冲区不足返回 `null`。

## 使用示例

```cp
import { SseParser, SseEvent, formatSseEvent } from "cap:sse_parser";

const stream = "data: {\"delta\":\"hi\"}\n\ndata: [DONE]\n\n";
let parser = SseParser.init(stream);
while (ev := parser.next()) {
    // ev.event_type == "message", ev.data == "{\"delta\":\"hi\"}" ...
}
```

## 上游行为契约与流式边界注意事项

1. **LLM 流式增量输出与 `[DONE]` 哨兵**：
   - OpenAI / Anthropic 等流式 API 均采用 `data: {...}\n\n` 帧格式；`SseParser.next()` 仅在遇到空行时派发含至少一条 `data:` 的事件，纯注释心跳（`: keep-alive`）自动忽略，未以空行结束的半截帧不会提前误派发。
2. **多行 `data:` 拼接缓冲生命周期**：
   - 单条事件内多行 `data:` 使用 `SseParser` 内部 4096 字节缓冲以 `\n` 拼接，下次调用 `next()` 会覆盖该内部缓冲；若需跨事件保留 `ev.data`，调用方应及时拷贝或消费。
3. **重连元数据 (`lastEventId` & `retry_ms`)**：
   - `id:` 含 `\0` NUL 字节按 WHATWG §9.2.6 严格忽略且不覆盖既有 `lastEventId()`；`retry:` 非纯 ASCII 数字时静默忽略，适合断线重连携带 `Last-Event-ID`。
