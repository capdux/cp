[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cliui

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation multi-column CLI layout engine with ANSI-aware wrapping, alignment, and padding in pure CP

## 安装与导入

```bash
cap add cliui
```

```cp
import { alignToU8 } from "cliui";
```

## 核心导出 API

- `export enum Align`
- `export fn alignToU8(a: Align): u8;`
- `export fn formatTwoCol(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cliui@1.0.0`)，许可证：MIT
