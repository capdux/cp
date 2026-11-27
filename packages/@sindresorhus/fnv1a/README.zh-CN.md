[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @sindresorhus/fnv1a

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap FNV-1 and FNV-1a 32-bit and 64-bit streaming non-cryptographic hash functions for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a)

## 安装与导入

```bash
cap add @sindresorhus/fnv1a
```

```cp
import { fnv64OffsetBasis } from "@sindresorhus/fnv1a";
```

## 核心导出 API

- `export fn fnv64OffsetBasis(): u64;`
- `export fn fnv64Prime(): u64;`
- `export fn mulWrap32(a: u32, b: u32): u32;`
- `export fn mulFnv64Prime(h: u64): u64;`
- `export fn fnv1a32Update(state: u32, data: &[]u8): u32;`
- `export fn fnv1a32(data: &[]u8): u32;`
- `export fn fnv132(data: &[]u8): u32;`
- `export fn fnv1a64Update(state: u64, data: &[]u8): u64;`
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn fnv164(data: &[]u8): u64;`
- `export fn formatHex64(h: u64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@sindresorhus/fnv1a@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`const-fnv1a-hash@1.0.0`)，许可证：MIT
