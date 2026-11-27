[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# smart-buffer

> 版本：`4.2.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations

## 安装与导入

```bash
cap add smart-buffer
```

```cp
import { sbufMin } from "smart-buffer";
```

## 核心导出 API

- `export fn sbufMin(a: usize, b: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`smart-buffer@4.2.0`)（https://github.com/JoshGlazebrook/smart-buffer），许可证：MIT
