[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# slotmap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation generational-index slot map (SlotKey, SlotMapI64, SecondaryMapI64) with O(1) free-list and strict ABA protection (Rust slotmap v1 parity) for CP

## 安装与导入

```bash
cap add slotmap
```

```cp
import { slmIsOccupiedVersion } from "slotmap";
```

## 核心导出 API

- `export fn slmIsOccupiedVersion(version: u32): bool;`
- `export fn slmAllocVersion(cur_version: u32): u32;`
- `export fn slmFreeVersion(cur_version: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`slotmap@1.0.0`)，许可证：MIT
