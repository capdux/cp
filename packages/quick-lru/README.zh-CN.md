[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# quick-lru

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap two-generation QuickLRU cache (QuickLruI64) with O(1) rotation eviction (clean-room port of npm quick-lru)

## 安装与导入

```bash
cap add quick-lru
```

```cp
import { qlruStrEq } from "quick-lru";
```

## 核心导出 API

- `export fn qlruStrEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`quick-lru@1.0.0`)，许可证：MIT
