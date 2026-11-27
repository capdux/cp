[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jest-diff

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP Jest-style unified diff engine with ANSI coloring (clean-room MIT implementation of npm jest-diff)

## 安装与导入

```bash
cap add jest-diff
```

```cp
import { diffStats } from "jest-diff";
```

## 核心导出 API

- `export fn diffStats(expected: string, received: string): DiffSummary;`
- `export fn diffLinesUnified(expected: string, received: string, opts: DiffOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jest-diff@1.0.0`)，许可证：MIT
