[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bytemuck

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap safe POD byte reinterpretation, LE/BE integer/float byte casting, and alignment/size guard checks (crates.io bytemuck equivalent)

## 安装与导入

```bash
cap add bytemuck
```

```cp
import { checkCastLayout } from "bytemuck";
```

## 核心导出 API

- `export fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8;`
- `export fn isAligned(addr: usize, align_req: usize): bool;`
- `export fn fillZeroed(buf: []u8);`
- `export fn zeroedU8(): u8;`
- `export fn zeroedU16(): u16;`
- `export fn zeroedU32(): u32;`
- `export fn zeroedU64(): u64;`
- `export fn zeroedI32(): i32;`
- `export fn zeroedI64(): i64;`
- `export fn zeroedF32(): f32;`
- `export fn zeroedF64(): f64;`
- `export fn writeU16Le(v: u16, out: []u8): bool;`
- `export fn readU16Le(buf: []u8): u16?;`
- `export fn writeU16Be(v: u16, out: []u8): bool;`
- `export fn readU16Be(buf: []u8): u16?;`
- `export fn writeU32Le(v: u32, out: []u8): bool;`
- `export fn readU32Le(buf: []u8): u32?;`
- `export fn writeU32Be(v: u32, out: []u8): bool;`
- `export fn readU32Be(buf: []u8): u32?;`
- `export fn writeU64Le(v: u64, out: []u8): bool;`
- `export fn readU64Le(buf: []u8): u64?;`
- `export fn writeU64Be(v: u64, out: []u8): bool;`
- `export fn readU64Be(buf: []u8): u64?;`
- `export fn writeF32Le(v: f32, out: []u8): bool;`
- `export fn readF32Le(buf: &[]u8): f32?;`
- `export fn writeF64Le(v: f64, out: []u8): bool;`
- `export fn readF64Le(buf: &[]u8): f64?;`
- `export fn bytesOfU16(v: u16): [2]u8;`
- `export fn bytesOfU32(v: u32): [4]u8;`
- `export fn bytesOfU64(v: u64): [8]u8;`
- `export fn bytesOfF32(v: f32): [4]u8;`
- `export fn bytesOfF64(v: f64): [8]u8;`
- `export fn fromBytesU16(buf: &[]u8): u16?;`
- `export fn fromBytesU32(buf: &[]u8): u32?;`
- `export fn fromBytesU64(buf: &[]u8): u64?;`
- `export fn fromBytesF32(buf: &[]u8): f32?;`
- `export fn fromBytesF64(buf: &[]u8): f64?;`
- `export fn podReadUnalignedU16(buf: &[]u8): u16?;`
- `export fn podReadUnalignedU32(buf: &[]u8): u32?;`
- `export fn podReadUnalignedU64(buf: &[]u8): u64?;`
- 完整签名请参阅 [src/index.d.cp](./src/index.d.cp)

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bytemuck@1.0.0`)，许可证：Zlib OR Apache-2.0 OR MIT
