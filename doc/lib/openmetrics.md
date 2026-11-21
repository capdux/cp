# `cap:openmetrics` 标准库文档

## 1. 概述

`cap:openmetrics` 是纯 CP 实现的工业级 Prometheus 0.0.4 与 OpenMetrics 1.0.0 文本暴露格式（Text Exposition Format）解析、校验与序列化标准库，支持通过 `import { ... } from "cap:openmetrics"` 或 `import cap from "cap"` 动态挂载 `cap.openmetrics` 使用。

核心能力：
- 解析 `# HELP`、`# TYPE`、`# UNIT` 元数据行与 `# EOF` 结束标记
- 解析带转义序列（`\"`、`\\`、`\\n`）的标签列表与可选时间戳（整数毫秒或浮点秒）
- 完整支持 `NaN`、`+Inf`、`-Inf` 特殊浮点值及常规整型/浮点值
- 自动将 `_bucket` / `_sum` / `_count` / `_total` / `_created` 归并至对应 `MetricFamily`
- 支持确定性 `serialize(alloc)` 往返序列化与 `deinit(alloc)` 零泄漏释放

## 2. 核心类型

- `MetricType`: 枚举，包含 `Counter`、`Gauge`、`Histogram`、`Summary`、`Untyped`
- `Label`: `{ name: []const u8, value: []const u8 }`，支持 `deinit(alloc)`
- `Sample`: `{ name: []const u8, labels: []Label, value: f64, timestamp_ms: i64? }`，支持 `deinit(alloc)`
- `MetricFamily`: `{ name: []const u8, help: []const u8?, unit: []const u8?, metric_type: MetricType, samples: []Sample }`，支持 `deinit(alloc)`
- `MetricsDocument`: `{ families: []MetricFamily, has_eof: bool }`，提供解析、查询与序列化方法

## 3. API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` | `parse(alloc, text): MetricsDocument!` | 解析 Prometheus / OpenMetrics 文本为 `MetricsDocument` |
| `MetricsDocument.parse` | `MetricsDocument.parse(alloc, text): MetricsDocument!` | 同上 |
| `MetricsDocument.findFamily` | `findFamily(self, name): (*const MetricFamily)?` | 按名称查找指标家族 |
| `MetricsDocument.serialize` | `serialize(self, alloc): []u8!` | 序列化为标准文本格式（含转义与 `# EOF`） |
| `MetricsDocument.deinit` | `deinit(self, alloc)` | 释放所有家族、样本与标签内存 |
| `MetricType.toString` | `toString(self): []const u8` | 返回小写类型名称字符串 |
| `MetricType.fromString` | `fromString(raw): MetricType?` | 从小写类型名称解析 `MetricType` |

## 4. 示例

```cp
import { parse, MetricType } from "cap:openmetrics";

const text =
    "# HELP http_requests_total Total HTTP requests\n" ++
    "# TYPE http_requests_total counter\n" ++
    "http_requests_total{method=\"GET\",status=\"200\"} 1024\n" ++
    "# EOF\n";

let doc = try parse(alloc, text);
defer doc.deinit(alloc);

if (fam := doc.findFamily("http_requests_total")) {
    // fam.metric_type == MetricType.Counter
    // fam.samples[0].value == 1024.0
}

const out = try doc.serialize(alloc);
defer alloc.free(out);
```
