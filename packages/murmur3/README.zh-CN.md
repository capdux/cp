[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# murmur3

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64) deterministic non-cryptographic hash functions for sharding, Kafka/Cassandra/ClickHouse partitioning

## 安装与导入

```bash
cap add murmur3
```

```cp
import { rotl32 } from "murmur3";
```

## 核心导出 API

- `export fn rotl32(x: u32, r: u5): u32;`
- `export fn rotl64(x: u64, r: u6): u64;`
- `export fn fmix32(h_in: u32): u32;`
- `export fn fmix64(k_in: u64): u64;`
- `export fn murmur3_32(data: string, seed: u32): u32;`
- `export fn murmur3_128(data: string, seed: u64): Hash128;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`murmur3@1.0.0`)，许可证：MIT
