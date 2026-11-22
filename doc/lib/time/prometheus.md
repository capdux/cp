# `cap:prometheus` — Prometheus / OpenMetrics 指标注册表、直方图与文本格式编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

> **模块路径**: `lib/prometheus/`  
> **导入协议**: `import { PromRegistry, MetricType, LabelPair, ParsedSample, PromError, parseExposition, freeParsedSamples } from "cap:time";`  
> **动态挂载**: `import cap from "cap";` -> `cap.prometheus.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:prometheus` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix` 或 `@import("bun")`）的 Prometheus 监控指标引擎，支持指标注册、多维标签管理、累计直方图统计及标准 `text/plain; version=0.0.4` 文本协议的双向编解码：

1. **强校验指标与标签模型**：
   - 指标名严格校验 `[a-zA-Z_:][a-zA-Z0-9_:]*`；
   - 标签名严格校验 `[a-zA-Z_][a-zA-Z0-9_]*` 且拒绝保留前缀 `__`；
   - 重复注册相同指标名时返回 `PromError.DuplicateMetric`。
2. **核心指标类型 (`MetricType`)**：
   - **Counter (`registerCounter` / `incCounter`)**：单调递增计数器，负数或 `NaN` 增量返回 `PromError.NegativeCounterIncrement`。
   - **Gauge (`registerGauge` / `setGauge` / `addGauge`)**：可升可降仪表盘，支持直接设值与正负增量。
   - **Histogram (`registerHistogram` / `observeHistogram`)**：校验边界桶严格升序且为有限值（否则返回 `PromError.UnsortedBuckets`），自动维护累计分桶（`le`）、`+Inf` 桶、`_sum` 与 `_count`。
3. **Text Exposition 编解码 (`exportText` / `parseExposition`)**：
   - `exportText(allocator)` 输出标准 Prometheus `text/plain; version=0.0.4` 格式（含 `# HELP`、`# TYPE`），自动转义标签值中的 `\\`、`\"`、`\n` 及帮助文本中的 `\\`、`\n`。
   - `parseExposition(allocator, text)` 解析文本格式样本行，提取指标名、标签键值对（含转义还原）、数值（支持 `+Inf` / `-Inf` / `NaN`）以及 `# HELP` / `# TYPE` 元数据（自动关联直方图 `_bucket` / `_sum` / `_count` 后缀），跳过普通注释行；通过 `freeParsedSamples(allocator, samples)` 释放全部堆内存。

---

## 二、 核心 API 速查表 (`lib/prometheus/prometheus.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `PromError` | `InvalidMetricName`, `InvalidLabelName`, `NegativeCounterIncrement`, `DuplicateMetric`, `UnsortedBuckets`, `ParseError`, `OutOfMemory` |
| `MetricType` | `Counter`, `Gauge`, `Histogram` |
| `LabelPair` | `{ name: []const u8, value: []const u8 }` |
| `ParsedSample` | `{ name: []const u8, labels: []LabelPair, value: f64, help: ([]const u8)?, metric_type: MetricType? }` |
| `PromRegistry.init(allocator)` | 创建空指标注册表 |
| `PromRegistry.deinit()` | 释放全部已注册指标、序列与标签内存 |
| `PromRegistry.registerCounter(name, help)` | 注册 Counter 指标 |
| `PromRegistry.incCounter(name, labels, delta)` | Counter 按标签组合增加 `delta`（`delta < 0` 返回 `NegativeCounterIncrement`） |
| `PromRegistry.registerGauge(name, help)` | 注册 Gauge 指标 |
| `PromRegistry.setGauge(name, labels, value)` | 设置 Gauge 指标当前值 |
| `PromRegistry.addGauge(name, labels, delta)` | 对 Gauge 指标增减 `delta` |
| `PromRegistry.registerHistogram(name, help, buckets)` | 注册 Histogram 指标（校验 `buckets` 严格升序） |
| `PromRegistry.observeHistogram(name, labels, value)` | 记录一次观测值，更新累计分桶、`+Inf`、`_sum` 与 `_count` |
| `PromRegistry.exportText(allocator)` | 导出为 `text/plain; version=0.0.4` 字节切片（调用方负责 `allocator.free`） |
| `parseExposition(allocator, text)` | 解析文本格式指标为 `[]ParsedSample` |
| `freeParsedSamples(allocator, samples)` | 释放 `parseExposition` 返回的样本切片及内部字符串/标签内存 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    PromRegistry,
    LabelPair,
    parseExposition,
    freeParsedSamples,
} from "cap:time";

fn demo(alloc: std.mem.Allocator): !void {
    let reg = PromRegistry.init(alloc);
    defer reg.deinit();

    reg.registerCounter("http_requests_total", "Total HTTP requests")?;
    const lbls: [2]LabelPair = [
        LabelPair{ name: "method", value: "GET" },
        LabelPair{ name: "path", value: "/api/v1" },
    ];
    reg.incCounter("http_requests_total", &lbls, 1.0)?;

    const buckets: [3]f64 = [0.05, 0.1, 0.5];
    reg.registerHistogram("http_duration_seconds", "Request latency", &buckets)?;
    reg.observeHistogram("http_duration_seconds", &lbls, 0.08)?;

    const text = reg.exportText(alloc)?;
    defer alloc.free(text);

    const parsed = parseExposition(alloc, text)?;
    defer freeParsedSamples(alloc, parsed);
}
```
