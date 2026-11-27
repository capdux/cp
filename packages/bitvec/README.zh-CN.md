[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bitvec

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap arbitrary-length bit vector with popcount, find-first-one, bitwise ops and shifts (clean-room CP implementation of crates.io bitvec)

## 安装与导入

```bash
cap add bitvec
```

```cp
import { bvPopcountU64 } from "bitvec";
```

## 核心导出 API

- `export fn bvPopcountU64(x: u64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bitvec@1.0.0`)，许可证：MIT
