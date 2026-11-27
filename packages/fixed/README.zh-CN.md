[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fixed

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation signed 64-bit Q32.32 deterministic fixed-point arithmetic engine with overflow-free 128-bit split mul/div, Newton sqrt, lerp, and clamp (Rust fixed parity) for CP

## 安装与导入

```bash
cap add fixed
```

```cp
import { absU64 } from "fixed";
```

## 核心导出 API

- `export fn absU64(v: i64): u64;`
- `export fn divShift32U64(a: u64, b: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`fixed@1.0.0`)，许可证：MIT
