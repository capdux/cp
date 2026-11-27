[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-rational

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation exact rational number (Ratio64) arithmetic library with canonical reduction via @cap/num-integer for CP.

## 安装与导入

```bash
cap add num-rational
```

```cp
import { nratAbsU64 } from "num-rational";
```

## 核心导出 API

- `export fn nratAbsU64(v: i64): u64;`
- `export fn nratReduce(numer: i64, denom: i64): Ratio64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-rational@1.0.0`)，许可证：MIT OR Apache-2.0
