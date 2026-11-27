[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nohash-hasher

> 版本：`0.2.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-overhead identity integer hasher and Fibonacci/SplitMix64 integer key mixers for hash maps

## 安装与导入

```bash
cap add nohash-hasher
```

```cp
import { newIntHasher } from "nohash-hasher";
```

## 核心导出 API

- `export fn newIntHasher(): IntHasher;`
- `export fn nohashU64(key: u64): u64;`
- `export fn nohashU32(key: u32): u64;`
- `export fn fibBucketIndex(key: u64, bits: u6): usize;`
- `export fn mixU64(x: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`nohash-hasher@0.2.0`)（https://github.com/paritytech/nohash-hasher），许可证：Apache-2.0
