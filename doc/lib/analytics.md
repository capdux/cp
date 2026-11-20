# `cap:analytics` — 运行时遥测收集与脱敏批处理标准库

## 1. 概述
`cap:analytics`（同时支持 `import cap from "cap"` 动态挂载为 `cap.analytics`）是纯 CP 实现的工业级运行时遥测（Telemetry）事件收集、有界环形缓冲批处理、敏感字段自动脱敏与 RFC 8259 JSON / NDJSON 批量序列化标准库。

核心特性：
- **确定性所有权与零泄漏**：事件名称、会话/用户标识与字符串属性均由收集器独立拥有，通过 `flushBatch` 移交批次所有权，并在 `deinit()` / `freeBatch()` 中确定性释放。
- **敏感字段自动脱敏**：属性键名大小写不敏感匹配 `password`、`passwd`、`secret`、`token`、`api_key`、`apikey`、`private_key`、`access_key`、`credential`、`authorization` 时，值自动替换为 `"[REDACTED]"`。
- **有界环形缓冲与背压策略**：支持 `DropOldest`、`DropNewest` 与 `RejectWhenFull` 三种溢出策略。
- **紧凑序列化**：提供 `encodeBatchJson`（JSON 数组）与 `encodeBatchNdjson`（NDJSON），完整支持 RFC 8259 控制字符转义。

## 2. 核心类型
- `AnalyticsError`：错误集合（`EmptyEventName`, `EventNameTooLong`, `EmptyPropKey`, `PropKeyTooLong`, `TooManyProps`, `InvalidConfig`, `BufferFull`, `OutOfMemory`）。
- `DropPolicy`：环形缓冲满时的背压策略（`DropOldest`, `DropNewest`, `RejectWhenFull`）。
- `PropKind` / `PropValue` / `EventProp`：类型化事件属性（`String` / `Int` / `Float` / `Bool`），提供 `EventProp.str` / `.int` / `.float` / `.boolean` 构造器。
- `AnalyticsEvent`：遥测事件结构体（`name`, `distinct_id`, `session_id`, `timestamp_ms`, `props`）。
- `CollectorConfig` / `AnalyticsCollector`：有界环形缓冲遥测收集器。

## 3. API 速查
| 符号 | 说明 |
| :--- | :--- |
| `AnalyticsCollector.init(alloc, config)` | 初始化指定容量与背压策略的收集器 |
| `collector.track(name, distinct_id, session_id, ts_ms, props)` | 校验、脱敏并写入一条事件 |
| `collector.flushBatch(max_count)` | 按 FIFO 顺序拉取最多 `max_count` 条事件 |
| `collector.freeBatch(batch)` | 释放 `flushBatch` 返回的事件批次内存 |
| `collector.pendingCount()` / `droppedCount()` / `capacity()` | 查询待处理数、丢弃计数与容量 |
| `collector.clear()` / `collector.deinit()` | 清空缓冲或销毁收集器 |
| `isSensitiveKey(key)` | 判断属性键名是否属于敏感凭据 |
| `encodeBatchJson(alloc, events)` | 将事件批次编码为紧凑 JSON 数组 |
| `encodeBatchNdjson(alloc, events)` | 将事件批次编码为 NDJSON 字符串 |

## 4. 示例
```cp
import { AnalyticsCollector, CollectorConfig, EventProp, encodeBatchJson } from "cap:analytics";

let collector = try AnalyticsCollector.init(alloc, CollectorConfig{
    max_events: 128,
    drop_policy: .DropOldest,
});
defer collector.deinit();

const props = [_]EventProp{
    EventProp.str("route", "/v1/login"),
    EventProp.str("api_key", "sk-live-123456"),
    EventProp.int("status", 200),
};
try collector.track("http_request", "user_42", "sess_9", 1700000000000, &props);

const batch = try collector.flushBatch(16);
defer collector.freeBatch(batch);

const payload = try encodeBatchJson(alloc, batch);
defer alloc.free(payload);
```
