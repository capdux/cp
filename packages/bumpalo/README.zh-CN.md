[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bumpalo

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation alignment-aware bump arena allocator (BumpArena, BumpSlice) with checkpoint/rewind and overflow protection (Rust bumpalo v3 parity) for CP

## 安装与导入

```bash
cap add bumpalo
```

```cp
import { bmpIsValidAlign } from "bumpalo";
```

## 核心导出 API

- `export fn bmpIsValidAlign(alignment: usize): bool;`
- `export fn bmpAlignUp(offset: usize, alignment: usize): usize?;`
- `export fn bmpHandleValid(handle: BumpSlice, cursor: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bumpalo@1.0.0`)，许可证：MIT
