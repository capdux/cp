[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zod

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap type-safe schema validation framework for CP (clean-room MIT implementation of npm zod v3).

## 安装与导入

```bash
cap add zod
```

```cp
import { zString } from "zod";
```

## 核心导出 API

- `export fn zString(raw: string, min_len: usize, max_len: usize): bool;`
- `export fn zNumber(raw: string, min_v: f64, max_v: f64): bool;`
- `export fn zEmail(s: string): bool;`
- `export fn zUrl(s: string): bool;`
- `export fn zUuid(s: string): bool;`
- `export fn zEnum(s: string, csv: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`zod@1.0.0`)，许可证：MIT
