[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zerocopy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap zero-copy prefix/suffix byte slice cursor readers & writers with layout validation (crates.io zerocopy equivalent, reuses bytemuck)

## 安装与导入

```bash
cap add zerocopy
```

```cp
import { isAlignedSlice } from "zerocopy";
```

## 核心导出 API

- `export fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool;`
- `export fn readFromPrefix(buf: &[]u8, n: usize): SplitBytes?;`
- `export fn readFromSuffix(buf: &[]u8, n: usize): SplitBytes?;`
- `export fn readU16LeFromPrefix(buf: &[]u8): PrefixU16?;`
- `export fn readU16BeFromPrefix(buf: &[]u8): PrefixU16?;`
- `export fn readU32LeFromPrefix(buf: &[]u8): PrefixU32?;`
- `export fn readU32BeFromPrefix(buf: &[]u8): PrefixU32?;`
- `export fn readU64LeFromPrefix(buf: &[]u8): PrefixU64?;`
- `export fn readU64BeFromPrefix(buf: &[]u8): PrefixU64?;`
- `export fn readU16LeFromSuffix(buf: &[]u8): PrefixU16?;`
- `export fn readU32LeFromSuffix(buf: &[]u8): PrefixU32?;`
- `export fn readU64LeFromSuffix(buf: &[]u8): PrefixU64?;`
- `export fn writeToPrefix(dst: []u8, src: &[]u8): ([]u8)?;`
- `export fn writeToSuffix(dst: []u8, src: &[]u8): ([]u8)?;`
- `export fn refFromBytesU16(buf: &[]u8): u16?;`
- `export fn refFromBytesU32(buf: &[]u8): u32?;`
- `export fn refFromBytesU64(buf: &[]u8): u64?;`
- `export fn refFromBytesSliceU32(buf: &[]u8): (&[]u32)?;`
- `export fn sliceAsBytesU16(src: &[]u16): &[]u8;`
- `export fn sliceAsBytesU32(src: &[]u32): &[]u8;`
- `export fn sliceAsBytesU64(src: &[]u64): &[]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zerocopy@1.0.0`)，许可证：BSD-2-Clause OR Apache-2.0 OR MIT
