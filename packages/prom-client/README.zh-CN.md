[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# prom-client

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Prometheus / OpenMetrics text exposition format serializer with strict metric/label validation and escaping.

## 安装与导入

```bash
cap add prom-client
```

```cp
import { isValidMetricName } from "prom-client";
```

## 核心导出 API

- `export fn isValidMetricName(name: &[]u8): bool;`
- `export fn isValidLabelName(name: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`prom-client@1.0.0`)，许可证：Apache-2.0
