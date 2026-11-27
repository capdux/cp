[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# prom-client

纯 CP 实现的零堆分配 **Prometheus / OpenMetrics 文本暴露格式**序列化器，内置严格的指标名/标签名合法性校验、自动转义与缓冲区溢出保护。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/prom-client) |
| **原包名与版本** | `prom-client`（v15 对齐） |
| **上游仓库** | https://github.com/siimon/prom-client |
| **CP 包名与版本** | `prom-client`（`1.0.0`） |
| **开源协议** | MIT / Apache-2.0 |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import client from "prom-client";

// CP (cap)
import {
    Label,
    PromWriter,
    isValidMetricName,
    isValidLabelName,
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    METRIC_SUMMARY,
} from "prom-client";
```

## 安装

```bash
cap add prom-client
```

## 核心特性与安全亮点

- **零堆分配**：`PromWriter` 直接向调用方提供的 `[]u8` 缓冲区输出 `/metrics` 文本载荷，适合高频 Prometheus 拉取端点。
- **严格标识符校验**：`isValidMetricName` 校验 `[a-zA-Z_:][a-zA-Z0-9_:]*`；`isValidLabelName` 校验 `[a-zA-Z_][a-zA-Z0-9_]*` 并拒绝保留的 `__*` 前缀。
- **自动防注入转义**：自动转义 `HELP` 说明与标签值中的 `\`、`\n`、`"`，防止指标文本格式注入。
- **粘性溢出标志**：缓冲区满时自动置位 `overflow = true`，安全拒绝后续写入，绝不越界。

## 快速上手

```typescript
import { Label, PromWriter, METRIC_COUNTER, METRIC_GAUGE } from "prom-client";

fn formatMetrics(buf: []u8): &[]u8 {
    let w = PromWriter.init(buf);

    _ = w.writeHelp("http_requests_total", "Total HTTP requests processed");
    _ = w.writeType("http_requests_total", METRIC_COUNTER);

    const labels: [2]Label = [
        Label { key: "method", val: "GET" },
        Label { key: "status", val: "200" },
    ];
    _ = w.writeSampleInt("http_requests_total", labels[0..], 4215);

    _ = w.writeHelp("active_agents", "Currently active BEAM agents");
    _ = w.writeType("active_agents", METRIC_GAUGE);
    const empty_labels: [0]Label = [];
    _ = w.writeSampleWithTs("active_agents", empty_labels[0..], 1024, 1700000000000);

    return w.written();
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `METRIC_COUNTER` / `METRIC_GAUGE` / `METRIC_HISTOGRAM` / `METRIC_SUMMARY` | `0` / `1` / `2` / `3` | `writeType` 使用的指标类型常量 |
| `Label` | `class { key: &[]u8, val: &[]u8 }` | 标签键值对 |
| `isValidMetricName` / `isValidLabelName` | `(name: &[]u8) -> bool` | 指标名与标签名合法性校验 |
| `PromWriter.init` | `(buf: []u8) -> PromWriter` | 基于调用方缓冲区创建写入器 |
| `PromWriter.writeHelp` / `writeType` | 输出 `# HELP` / `# TYPE` 行 | 写入指标元数据 |
| `PromWriter.writeSampleInt` / `writeSampleWithTs` | 输出整数样本行 | 可选附带毫秒时间戳 |
| `PromWriter.written` / `len` / `isOverflow` | 获取输出切片、长度与溢出标志 | 零拷贝读取已写入数据 |

## 开源协议

MIT / Apache-2.0
