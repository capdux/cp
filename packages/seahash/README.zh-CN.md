[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# seahash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP 4-lane parallel SeaHash 64-bit fast non-cryptographic hash function (crates.io seahash equivalent)

## 安装与导入

```bash
cap add seahash
```

```cp
import { seaU64 } from "seahash";
```

## 核心导出 API

- `export fn seaU64(hi: u32, lo: u32): u64;`
- `export fn seaMul64(a: u64, b: u64): u64;`
- `export fn seahashDiffuse(x_in: u64): u64;`
- `export fn seahashBytes(buf: []u8): u64;`
- `export fn seahashSeeded(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`seahash@1.0.0`)，许可证：MIT
