[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bincode

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bincode v2 compact binary serializer & zero-copy decoder (LE varint, ZigZag signed ints, length-prefixed strings, Option, fixed LE integers — Rust bincode parity) for CP

## 安装与导入

```bash
cap add bincode
```

```cp
import { zigzagEncode } from "bincode";
```

## 核心导出 API

- `export fn zigzagEncode(v: i64): u64;`
- `export fn zigzagDecode(u: u64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bincode@1.0.0`)，许可证：MIT
