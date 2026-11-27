[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bit_field

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bit and bit-range manipulation utilities for u64, u32, and byte slices in pure CP

## 安装与导入

```bash
cap add bit_field
```

```cp
import { bfldMaskU64 } from "bit_field";
```

## 核心导出 API

- `export fn bfldMaskU64(width: u6): u64;`
- `export fn bfldMaskU32(width: u5): u32;`
- `export fn getBitU64(val: u64, bit: u6): bool;`
- `export fn setBitU64(val: u64, bit: u6, on: bool): u64;`
- `export fn getBitsU64(val: u64, start: u6, end: u6): u64;`
- `export fn setBitsU64(val: u64, start: u6, end: u6, bits: u64): u64;`
- `export fn getBitU32(val: u32, bit: u5): bool;`
- `export fn setBitU32(val: u32, bit: u5, on: bool): u32;`
- `export fn getBitsU32(val: u32, start: u5, end: u5): u32;`
- `export fn setBitsU32(val: u32, start: u5, end: u5, bits: u32): u32;`
- `export fn getBitSlice(bytes: &[]u8, bit_index: usize): bool;`
- `export fn setBitSlice(bytes: []u8, bit_index: usize, on: bool);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bit_field@1.0.0`)，许可证：MIT
