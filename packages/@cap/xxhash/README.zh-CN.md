[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/xxhash

> 版本：`0.8.3` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

xxHash 0.8.3 extremely fast non-cryptographic hash algorithm (XXH32/XXH64/XXH3) for CP

## 安装与导入

```bash
cap add @cap/xxhash
```

```cp
import { XXH_versionNumber } from "@cap/xxhash";
```

## 核心导出 API

- `export fn XXH_versionNumber(): u32;`
- `export fn XXH32(input: [*]&u8!, length: usize, seed: u32): u32;`
- `export fn XXH64(input: [*]&u8!, length: usize, seed: u64): u64;`
- `export fn XXH3_64bits(input: [*]&u8!, length: usize): u64;`
- `export fn versionNumber(): u32;`
- `export fn xxh32(data: &[]u8, seed: u32): u32;`
- `export fn xxh64(data: &[]u8, seed: u64): u64;`
- `export fn xxh3_64bits(data: &[]u8): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`xxHash@0.8.3`)，许可证：BSD-2-Clause
