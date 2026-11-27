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
- `export fn writeU16Le(v: u16, out: []u8): bool;`
- `export fn readU16Le(buf: []u8): u16?;`
- `export fn writeU32Le(v: u32, out: []u8): bool;`
- `export fn readU32Le(buf: []u8): u32?;`
- `export fn writeU32Be(v: u32, out: []u8): bool;`
- `export fn readU32Be(buf: []u8): u32?;`
- `export fn writeU64Le(v: u64, out: []u8): bool;`
- `export fn readU64Le(buf: []u8): u64?;`
- `export fn castU32SliceToBytesLe(src: []u32, dst: []u8): usize?;`
- `export fn castBytesToU32SliceLe(src: []u8, dst: []u32): usize?;`
- `export fn swapBytesU32(v: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bytemuck@1.0.0`)，许可证：Zlib OR Apache-2.0 OR MIT
