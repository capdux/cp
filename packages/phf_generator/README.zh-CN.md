[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# phf_generator

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CHD Displace-Hash-Compress perfect hash table builder and O(1) lookup for CP

## 安装与导入

```bash
cap add phf_generator
```

```cp
import { add64 } from "phf_generator";
```

## 核心导出 API

- `export fn add64(a: u64, b: u64): u64;`
- `export fn mul64(a: u64, b: u64): u64;`
- `export fn mix64(z_in: u64): u64;`
- `export fn hashKey(key: string, seed: u64): PhfHashes;`
- `export fn displace(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize;`
- `export fn buildPhfWithSeed(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;`
- `export fn buildPhf(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;`
- `export fn lookupIndex(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`phf_generator@1.0.0`)，许可证：MIT
