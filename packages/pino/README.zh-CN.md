[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pino

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap structured NDJSON logger with numeric levels (10..60), safe JSON escaping, and child fields (clean-room port of npm pino)

## 安装与导入

```bash
cap add pino
```

```cp
import { pinoPush } from "pino";
```

## 核心导出 API

- `export fn pinoPush(out: []u8, pos: usize, s: string): usize;`
- `export fn pinoPushJsonStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pinoPushU64(out: []u8, pos: usize, v: u64): usize;`
- `export fn pinoPushI64(out: []u8, pos: usize, v: i64): usize;`
- `export fn levelLabel(level: u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pino@1.0.0`)，许可证：MIT
