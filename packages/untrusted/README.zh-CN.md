[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# untrusted

> 版本：`1.0.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap big-endian/little-endian binary codecs & bounds-checked untrusted ByteReader/ByteWriter (inspired by Rust byteorder + untrusted)

## 安装与导入

```bash
cap add untrusted
```

```cp
import { readU16Be } from "untrusted";
```

## 核心导出 API

- `export fn readU16Be(buf: string, off: usize): u16?;`
- `export fn readU16Le(buf: string, off: usize): u16?;`
- `export fn readU32Be(buf: string, off: usize): u32?;`
- `export fn readU32Le(buf: string, off: usize): u32?;`
- `export fn readU64Be(buf: string, off: usize): u64?;`
- `export fn readU64Le(buf: string, off: usize): u64?;`
- `export fn writeU16Be(out: []u8, off: usize, v: u16): bool;`
- `export fn writeU16Le(out: []u8, off: usize, v: u16): bool;`
- `export fn writeU32Be(out: []u8, off: usize, v: u32): bool;`
- `export fn writeU32Le(out: []u8, off: usize, v: u32): bool;`
- `export fn writeU64Be(out: []u8, off: usize, v: u64): bool;`
- `export fn writeU64Le(out: []u8, off: usize, v: u64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`untrusted@1.0.1`)，许可证：ISC
- **上游生态**：`crates.io` (`byteorder@1.0.1`)，许可证：MIT
