[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bit_field

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bit and bit-range manipulation utilities for u64, u32, and byte slices in pure CP

## 安装与导入

```bash
cap add bit_field
```

```cp
import { bitLengthU8 } from "bit_field";
```

## 核心导出 API

- `export fn bitLengthU8(): usize;`
- `export fn bitLengthU16(): usize;`
- `export fn bitLengthU32(): usize;`
- `export fn bitLengthU64(): usize;`
- `export fn bitLengthUsize(): usize;`
- `export fn bitLengthSlice(bytes: &[]u8): usize;`
- `export fn bfldMaskU8(width: u3): u8;`
- `export fn bfldMaskU16(width: u4): u16;`
- `export fn bfldMaskU32(width: u5): u32;`
- `export fn bfldMaskU64(width: u6): u64;`
- `export fn bfldFullMaskU64(width: u8): u64;`
- `export fn getBitU8(val: u8, bit: u3): bool;`
- `export fn setBitU8(val: u8, bit: u3, on: bool): u8;`
- `export fn getBitsU8(val: u8, start: u3, end: u3): u8;`
- `export fn setBitsU8(val: u8, start: u3, end: u3, bits: u8): u8;`
- `export fn getBitU16(val: u16, bit: u4): bool;`
- `export fn setBitU16(val: u16, bit: u4, on: bool): u16;`
- `export fn getBitsU16(val: u16, start: u4, end: u4): u16;`
- `export fn setBitsU16(val: u16, start: u4, end: u4, bits: u16): u16;`
- `export fn getBitU64(val: u64, bit: u6): bool;`
- `export fn setBitU64(val: u64, bit: u6, on: bool): u64;`
- `export fn getBitsU64(val: u64, start: u6, end: u6): u64;`
- `export fn setBitsU64(val: u64, start: u6, end: u6, bits: u64): u64;`
- `export fn getBitsRangeU64(val: u64, start: u8, end: u8): u64;`
- `export fn setBitsRangeU64(val: u64, start: u8, end: u8, bits: u64): u64;`
- `export fn getBitU32(val: u32, bit: u5): bool;`
- `export fn setBitU32(val: u32, bit: u5, on: bool): u32;`
- `export fn getBitsU32(val: u32, start: u5, end: u5): u32;`
- `export fn setBitsU32(val: u32, start: u5, end: u5, bits: u32): u32;`
- `export fn getBitUsize(val: usize, bit: u8): bool;`
- `export fn setBitUsize(val: usize, bit: u8, on: bool): usize;`
- `export fn getBitsUsize(val: usize, start: u8, end: u8): usize;`
- `export fn setBitsUsize(val: usize, start: u8, end: u8, bits: usize): usize;`
- `export fn getBitSlice(bytes: &[]u8, bit_index: usize): bool;`
- `export fn setBitSlice(bytes: []u8, bit_index: usize, on: bool);`
- `export fn toggleBitSlice(bytes: []u8, bit_index: usize);`
- `export fn getBitsSlice(bytes: &[]u8, start: usize, end: usize): u64;`
- `export fn setBitsSlice(bytes: []u8, start: usize, end: usize, value: u64);`
- `export fn countOnesSlice(bytes: &[]u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bit_field@1.0.0`)，许可证：MIT
