[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dlv-list

> 版本：`0.5.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Vec-backed doubly linked list with stable u32 handles and O(1) slot reuse

## 安装与导入

```bash
cap add dlv-list
```

```cp
import { dlvlZeroI64 } from "dlv-list";
```

## 核心导出 API

- `export fn dlvlZeroI64(): [DLV_CAP]i64;`
- `export fn dlvlNoneU32(): [DLV_CAP]u32;`
- `export fn dlvlFalseBool(): [DLV_CAP]bool;`
- `export fn dlvlAllocSlot(list: &mut DlvListI64): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`dlv-list@0.5.2`)（https://github.com/sgodwinson/dlv-list），许可证：MIT
