# `cap:otlp` — OpenTelemetry OTLP 遥测标准库

## 1. 概述

`lib/otlp`（对外协议 `cap:otlp` 与 `cap.otlp`）提供纯 CP 实现的工业级 OpenTelemetry OTLP 遥测数据模型、W3C TraceContext / Baggage 上下文传播与 OTLP/HTTP JSON 导出器，覆盖 Traces、Metrics 与 Logs 三大信号。

## 2. 核心类型

- **传播上下文 (`context.cp`)**：
  - `OtlpError`：`InvalidTraceParent`、`InvalidVersion`、`ZeroTraceId`、`ZeroSpanId`、`InvalidHex`、`InvalidBaggage`、`BaggageLimitExceeded`、`OutOfMemory`
  - `TraceParent`：`version: u8`、`trace_id: [16]u8`、`span_id: [8]u8`、`trace_flags: u8`；方法 `isSampled()`、`newChildSpan(child_span_id)`
  - `BaggageEntry`：`key: []const u8`、`value: []const u8`、`metadata: ([]const u8)?`
- **遥测模型 (`model.cp`)**：
  - `AnyValue` / `KeyValue` / `ResourceInfo`
  - `SpanKind` / `SpanStatusCode` / `SpanStatus` / `SpanEvent` / `SpanLink` / `OtlpSpan`
  - `MetricKind` / `MetricDataPoint` / `OtlpMetric`
  - `SeverityNumber` / `OtlpLogRecord`

## 3. API 速查

| 函数 | 说明 |
| --- | --- |
| `parseTraceParent(header)` | 解析并严格校验 W3C `traceparent` 头（拦截全零 ID、非法版本 `ff` 与长度异常） |
| `formatTraceParent(alloc, tp)` | 格式化为 55 字节标准 `00-<trace_id>-<span_id>-<flags>` 字符串 |
| `parseBaggage(alloc, header)` | 解析 W3C `baggage` 头，支持百分号解码、`;metadata` 与上限防护 |
| `formatBaggage(alloc, entries)` | 编码并格式化为标准 `baggage` 头 |
| `freeBaggage(alloc, entries)` | 释放 `parseBaggage` 分配的切片与字段内存 |
| `encodeExportTraceRequestJson(alloc, resource, spans)` | 导出 OTLP/HTTP JSON `resourceSpans` 载荷 |
| `encodeExportMetricsRequestJson(alloc, resource, metrics)` | 导出 OTLP/HTTP JSON `resourceMetrics` 载荷 |
| `encodeExportLogsRequestJson(alloc, resource, logs)` | 导出 OTLP/HTTP JSON `resourceLogs` 载荷 |

## 4. 示例

```cp
import { parseTraceParent, formatTraceParent, encodeExportTraceRequestJson, ResourceInfo, OtlpSpan } from "cap:otlp";

const tp = try parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
const child = try tp.newChildSpan([8]u8{ 1, 2, 3, 4, 5, 6, 7, 8 });
const header = try formatTraceParent(alloc, &child);
defer alloc.free(header);
```
