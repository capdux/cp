[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/sse

Official Server-Sent Events (SSE) plugin with Last-Event-ID replay buffer & AI streaming helpers for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/sse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/sse
```

```cp
import {
    SseConfig,
    SseMessage,
    sseWriteRaw,
    sseWriteU64,
    sseParseU64,
    encodeSseEvent,
    encodeSseHeartbeat,
    sseResetBroker,
} from "@web/sse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SseConfig` | `class` | 字段: `endpoint_path: string, retry_ms: u32, cors_allow_origin: string` |
| `SseMessage` | `class` | 字段: `id: u64, event_name: string, data: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sseWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | 调用 `sseWriteRaw` 并返回 `bool`。 |
| `sseWriteU64` | `(dst: []u8, pos: &mut usize, val: u64): bool` | 调用 `sseWriteU64` 并返回 `bool`。 |
| `sseParseU64` | `(s: string): u64` | 调用 `sseParseU64` 并返回 `u64`。 |
| `encodeSseEvent` | `(id: u64, event_name: string, data: string, out_buf: []u8, pos: &mut usize): bool` | 通过 `encodeSseEvent` 编码或格式化输出，返回 `bool`。 |
| `encodeSseHeartbeat` | `(out_buf: []u8, pos: &mut usize): bool` | 通过 `encodeSseHeartbeat` 编码或格式化输出，返回 `bool`。 |
| `sseResetBroker` | `()` | 就地执行 `sseResetBroker` 操作。 |
| `ssePublish` | `(event_name: string, data: string): u64` | 调用 `ssePublish` 并返回 `u64`。 |
| `buildSseReplayResponse` | `(last_event_id: u64): Response` | 调用 `buildSseReplayResponse` 并返回 `Response`。 |
| `handleSseEndpoint` | `(ctx: &mut Context): Response` | 调用 `handleSseEndpoint` 并返回 `Response`。 |
| `createSsePlugin` | `(cfg: SseConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerSse` | `(app: &mut App, cfg: SseConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { encodeSseEvent } from "@web/sse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeSseEvent(0, "example", "example", out_buf[0..256], /* pos: &mut usize */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
