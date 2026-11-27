[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/sse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official Server-Sent Events (SSE) plugin with Last-Event-ID replay buffer & AI streaming helpers for cap:web

## 安装与导入

```bash
cap add @web/sse
```

```cp
import { sseWriteRaw } from "@web/sse";
```

## 核心导出 API

- `export fn sseWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;`
- `export fn sseWriteU64(dst: []u8, pos: &mut usize, val: u64): bool;`
- `export fn sseParseU64(s: string): u64;`
- `export fn encodeSseEvent(id: u64, event_name: string, data: string, out_buf: []u8, pos: &mut usize): bool;`
- `export fn encodeSseHeartbeat(out_buf: []u8, pos: &mut usize): bool;`
- `export fn sseResetBroker();`
- `export fn ssePublish(event_name: string, data: string): u64;`
- `export fn buildSseReplayResponse(last_event_id: u64): Response;`
- `export fn handleSseEndpoint(ctx: &mut Context): Response;`
- `export fn createSsePlugin(cfg: SseConfig): Plugin;`
- `export fn registerSse(app: &mut App, cfg: SseConfig): &mut App;`
