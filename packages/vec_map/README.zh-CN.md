[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# vec_map

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation O(1) direct-indexed sparse integer-key map (0..127) with 128-bit presence bitmap in CP

## 安装与导入

```bash
cap add vec_map
```

```cp
import { vmapMask } from "vec_map";
```

## 核心导出 API

- `export fn vmapMask(bit: usize): u64;`
- `export fn vmapHasBit(bits_lo: u64, bits_hi: u64, k: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`vec_map@1.0.0`)，许可证：MIT OR Apache-2.0
