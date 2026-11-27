[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# termtree

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CLI tree structure renderer with Unicode/ASCII glyph palettes and fixed-capacity arena (Rust termtree parity) for CP

## 安装与导入

```bash
cap add termtree
```

```cp
import { ttrAppendBytes } from "termtree";
```

## 核心导出 API

- `export fn ttrAppendBytes(out: []u8, pos: usize, src: string): usize?;`
- `export fn ttrAppendByte(out: []u8, pos: usize, b: u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`termtree@1.0.0`)，许可证：MIT
