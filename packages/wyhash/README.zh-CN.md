[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wyhash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Wang Yi wyhash v4 64-bit hash and WyRng PRNG with overflow-free 128-bit multiply in pure CP

## 安装与导入

```bash
cap add wyhash
```

```cp
import { makeU64 } from "wyhash";
```

## 核心导出 API

- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn mul128(a: u64, b: u64): Mul128;`
- `export fn wymum(a: u64, b: u64): u64;`
- `export fn read4(data: &[]u8, off: usize): u64;`
- `export fn read8(data: &[]u8, off: usize): u64;`
- `export fn wyhashBytes(data: &[]u8, seed: u64): u64;`
- `export fn wyhash(data: string, seed: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`wyhash@1.0.0`)，许可证：MIT
