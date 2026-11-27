[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ahash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation randomized high-speed non-cryptographic hasher with 4-key RandomState, folded multiply, and rotation rounds (Rust ahash parity) for CP

## 安装与导入

```bash
cap add ahash
```

```cp
import { hashBytes } from "ahash";
```

## 核心导出 API

- `export fn hashBytes(data: &[]u8, k0: u64, k1: u64): u64;`
- `export fn hashU64(v: u64, k0: u64, k1: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ahash@1.0.0`)，许可证：MIT
