[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# eventsource-parser

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:eventsource-parser` ([https://github.com/rexxars/eventsource-parser](https://github.com/rexxars/eventsource-parser))（原库名 `eventsource-parser` → CP 包名 `eventsource-parser`）

Zero-allocation W3C Server-Sent Events (`text/event-stream`) incremental streaming parser for CP.

## Features

- **Zero heap allocation**: fixed internal buffers (`data_buf[2048]`, `event_buf[128]`, `id_buf[128]`, `line_buf[512]`).
- **Byte-by-byte streaming**: feeds arbitrary TCP chunks via `pushByte(b)` with CRLF, LF, and CR line ending support.
- **Full W3C SSE semantics**: `event:`, multi-line `data:` joined by `\n`, persistent `id:`, `retry:` milliseconds, `:comment` heartbeat skipping, single-space stripping after `:`, and buffer overflow protection.

## Usage

```cp
import { SseParser } from "eventsource-parser";

let parser = SseParser.init();
const chunk = "data: {\"delta\":\"hi\"}\n\n";
let i: usize = 0;
while (i < chunk.len) {
    const st = parser.pushByte(chunk[i]);
    if (st == 1) {
        const kind = parser.eventType(); // "message"
        const payload = parser.eventData(); // "{\"delta\":\"hi\"}"
        parser.consumeEvent();
    }
    i += 1;
}
```
