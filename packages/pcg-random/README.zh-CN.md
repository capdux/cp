[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pcg-random

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG with stream separation, unbiased bounded sampling, O(log n) advance, and byte filling.

## 安装与导入

```bash
cap add pcg-random
```

```cp
import { pcgMult } from "pcg-random";
```

## 核心导出 API

- `export fn pcgMult(): u64;`
- `export fn defaultStream(): u64;`
- `export fn add64(a: u64, b: u64): u64;`
- `export fn mul64(a: u64, b: u64): u64;`
- `export fn rotr32(x: u32, rot: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pcg-random@1.0.0`)，许可证：MIT
