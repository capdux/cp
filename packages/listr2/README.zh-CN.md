[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# listr2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CLI task list state machine with pending/running/completed/skipped/failed lifecycle, retries, and ANSI rendering (npm listr2 equivalent, reuses picocolors)

## 安装与导入

```bash
cap add listr2
```

```cp
import { lstrPush } from "listr2";
```

## 核心导出 API

- `export fn lstrPush(out: []u8, pos: usize, s: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`listr2@1.0.0`)，许可证：MIT
