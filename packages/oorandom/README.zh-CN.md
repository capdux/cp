[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# oorandom

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap deterministic PCG32 (XSH RR 64/32) and dual-stream PCG64 PRNG in pure CP (modeled after Rust oorandom)

## 安装与导入

```bash
cap add oorandom
```

```cp
import { oorStep } from "oorandom";
```

## 核心导出 API

- `export fn oorStep(st: u64, inc: u64): u64;`
- `export fn oorOutput(oldstate: u64): u32;`
- `export fn oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`oorandom@1.0.0`)，许可证：MIT
