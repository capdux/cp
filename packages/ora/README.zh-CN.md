[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ora

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CLI terminal spinner animation and status line renderer (clean-room port of ora v8)

## 安装与导入

```bash
cap add ora
```

```cp
import { oraWriteSlice } from "ora";
```

## 核心导出 API

- `export fn oraWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn oraFrameCount(style: u8): usize;`
- `export fn oraFrameAt(style: u8, index: usize): string;`
- `export fn oraRenderStatus(symbol: string, msg: string, fallback: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ora@1.0.0`)，许可证：MIT
