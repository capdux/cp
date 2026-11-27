[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# foldhash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 64x64->128-bit folded-multiply non-cryptographic hasher with Fast and Quality modes (Rust foldhash parity) for CP

## 安装与导入

```bash
cap add foldhash
```

```cp
import { foldedMultiply } from "foldhash";
```

## 核心导出 API

- `export fn foldedMultiply(x: u64, y: u64): u64;`
- `export fn hashBytesFast(data: &[]u8, seed: u64): u64;`
- `export fn hashBytesQuality(data: &[]u8, seed: u64): u64;`
- `export fn hashU64(v: u64, seed: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`foldhash@1.0.0`)，许可证：Zlib
