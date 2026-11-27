[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# reed-solomon-erasure

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding and FEC reconstruction engine for CP

## 安装与导入

```bash
cap add reed-solomon-erasure
```

```cp
import { gfMul } from "reed-solomon-erasure";
```

## 核心导出 API

- `export fn gfMul(a: u8, b: u8): u8;`
- `export fn gfPow(a: u8, n: u32): u8;`
- `export fn gfInv(a: u8): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`reed-solomon-erasure@1.0.0`)，许可证：MIT
