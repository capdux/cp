[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# enumflags2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 64-bit type-safe enum bitflags set with valid-mask protection for CP.

## 安装与导入

```bash
cap add enumflags2
```

```cp
import { ef2Popcount64 } from "enumflags2";
```

## 核心导出 API

- `export fn ef2Popcount64(val: u64): usize;`
- `export fn ef2Ctz64(val: u64): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`enumflags2@1.0.0`)，许可证：MIT
