[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# postgres-date

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ / DATE / infinity parser with microsecond and timezone offset precision (clean-room MIT implementation of npm postgres-date, #505 / 112M wk)

## 安装与导入

```bash
cap add postgres-date
```

```cp
import { parsePgDate } from "postgres-date";
```

## 核心导出 API

- `export enum PgDateKind`
- `export fn parsePgDate(input: string): PgDateTime;`
- `export fn toUnixMs(dt: PgDateTime): i64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`postgres-date@1.0.0`)，许可证：MIT
