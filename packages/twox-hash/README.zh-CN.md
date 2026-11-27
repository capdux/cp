[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# twox-hash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP XXHash32 and XXHash64 fast non-cryptographic streaming hashers (crates.io twox-hash equivalent)

## 安装与导入

```bash
cap add twox-hash
```

```cp
import { xxhU64 } from "twox-hash";
```

## 核心导出 API

- `export fn xxhU64(hi: u32, lo: u32): u64;`
- `export fn xxhAdd32(a: u32, b: u32): u32;`
- `export fn xxhSub32(a: u32, b: u32): u32;`
- `export fn xxhMul32(a: u32, b: u32): u32;`
- `export fn xxhRol32(x: u32, r: u32): u32;`
- `export fn xxhAdd64(a: u64, b: u64): u64;`
- `export fn xxhSub64(a: u64, b: u64): u64;`
- `export fn xxhMul64(a: u64, b: u64): u64;`
- `export fn xxhRol64(x: u64, r: u64): u64;`
- `export fn xxhReadU32Le(buf: []u8, off: usize): u32;`
- `export fn xxhReadU64Le(buf: []u8, off: usize): u64;`
- `export fn xxh32Round(acc: u32, lane: u32): u32;`
- `export fn xxh64Prime1(): u64;`
- `export fn xxh64Prime2(): u64;`
- `export fn xxh64Prime3(): u64;`
- `export fn xxh64Prime4(): u64;`
- `export fn xxh64Prime5(): u64;`
- `export fn xxh64Round(acc: u64, lane: u64): u64;`
- `export fn xxh64MergeRound(acc: u64, val: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`twox-hash@1.0.0`)，许可证：MIT
