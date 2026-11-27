[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rand_hc

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HC-128 eSTREAM cryptographic stream cipher PRNG (Hongjun Wu specification) in pure CP

## 安装与导入

```bash
cap add rand_hc
```

```cp
import { rhcRotr32 } from "rand_hc";
```

## 核心导出 API

- `export fn rhcRotr32(x: u32, n: usize): u32;`
- `export fn rhcRotl32(x: u32, n: usize): u32;`
- `export fn rhcF1(x: u32): u32;`
- `export fn rhcF2(x: u32): u32;`
- `export fn rhcG1(x: u32, y: u32, z: u32): u32;`
- `export fn rhcG2(x: u32, y: u32, z: u32): u32;`
- `export fn rhcReadU32Le(seed: [32]u8, offset: usize): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rand_hc@1.0.0`)，许可证：MIT OR Apache-2.0
