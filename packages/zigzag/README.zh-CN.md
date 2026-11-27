[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zigzag

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 32-bit/64-bit ZigZag bijection and ZigZag-Varint streaming codec (Protobuf/Avro/Parquet/CBOR) in pure CP

## 安装与导入

```bash
cap add zigzag
```

```cp
import { encodeI8 } from "zigzag";
```

## 核心导出 API

- `export fn encodeI8(n: i8): u8;`
- `export fn decodeU8(u: u8): i8;`
- `export fn decodeI8(u: u8): i8;`
- `export fn encodeI16(n: i16): u16;`
- `export fn decodeU16(u: u16): i16;`
- `export fn decodeI16(u: u16): i16;`
- `export fn encodeI32(n: i32): u32;`
- `export fn decodeI32(u: u32): i32;`
- `export fn decodeU32(u: u32): i32;`
- `export fn encodeI64(n: i64): u64;`
- `export fn decodeI64(u: u64): i64;`
- `export fn decodeU64(u: u64): i64;`
- `export fn encodeI8Slice(src: &[]i8, dst: []u8): usize;`
- `export fn decodeU8Slice(src: &[]u8, dst: []i8): usize;`
- `export fn encodeI16Slice(src: &[]i16, dst: []u16): usize;`
- `export fn decodeU16Slice(src: &[]u16, dst: []i16): usize;`
- `export fn encodeI32Slice(src: &[]i32, dst: []u32): usize;`
- `export fn decodeU32Slice(src: &[]u32, dst: []i32): usize;`
- `export fn encodeI64Slice(src: &[]i64, dst: []u64): usize;`
- `export fn decodeU64Slice(src: &[]u64, dst: []i64): usize;`
- `export fn varintLen(n: i64): usize;`
- `export fn encodeZigzagVarint(n: i64, out: []u8): usize;`
- `export fn decodeZigzagVarint(data: &[]u8, start: usize): VarintResult;`
- `export fn encodeZigzagVarintSlice(src: &[]i64, dst: []u8): usize;`
- `export fn decodeZigzagVarintSlice(src: &[]u8, dst: []i64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zigzag@1.0.0`)，许可证：MIT
