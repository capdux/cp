[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nibble_vec

> 版本：`0.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 4-bit nibble vector for radix trees and prefix compression

## 安装与导入

```bash
cap add nibble_vec
```

```cp
import { nvecMaxNibbles } from "nibble_vec";
```

## 核心导出 API

- `export fn nvecMaxNibbles(): usize;`
- `export fn nvecGetRaw(data: [64]u8, idx: usize): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`nibble_vec@0.1.0`)（https://github.com/michaelsproul/rust_nibble_vec），许可证：MIT
