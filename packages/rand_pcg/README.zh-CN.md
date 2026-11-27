[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rand_pcg

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PCG32 (XSH-RR) pseudorandom number generator with independent streams and O(log N) jump-ahead (inspired by Rust rand_pcg)

## 安装与导入

```bash
cap add rand_pcg
```

```cp
import { wrapAdd64 } from "rand_pcg";
```

## 核心导出 API

- `export fn wrapAdd64(a: u64, b: u64): u64;`
- `export fn wrapMul64(a: u64, b: u64): u64;`
- `export fn rotr32(x: u32, r: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rand_pcg@1.0.0`)，许可证：MIT
