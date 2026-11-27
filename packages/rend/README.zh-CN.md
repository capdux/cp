[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rend

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation endian-aware primitive types (U16/U32/I32/U64/I64 Be/Le) and bounds-checked slice readers for zero-copy binary protocols in CP

## 安装与导入

```bash
cap add rend
```

```cp
import { rndPackU16Be } from "rend";
```

## 核心导出 API

- `export fn rndPackU16Be(v: u16): [2]u8;`
- `export fn rndPackU16Le(v: u16): [2]u8;`
- `export fn rndUnpackU16Be(b: [2]u8): u16;`
- `export fn rndUnpackU16Le(b: [2]u8): u16;`
- `export fn rndPackU32Be(v: u32): [4]u8;`
- `export fn rndPackU32Le(v: u32): [4]u8;`
- `export fn rndUnpackU32Be(b: [4]u8): u32;`
- `export fn rndUnpackU32Le(b: [4]u8): u32;`
- `export fn rndPackU64Be(v: u64): [8]u8;`
- `export fn rndPackU64Le(v: u64): [8]u8;`
- `export fn rndUnpackU64Be(b: [8]u8): u64;`
- `export fn rndUnpackU64Le(b: [8]u8): u64;`
- `export fn readU16Be(buf: &[]u8, offset: usize): u16?;`
- `export fn readU16Le(buf: &[]u8, offset: usize): u16?;`
- `export fn readU32Be(buf: &[]u8, offset: usize): u32?;`
- `export fn readU32Le(buf: &[]u8, offset: usize): u32?;`
- `export fn readU64Be(buf: &[]u8, offset: usize): u64?;`
- `export fn readU64Le(buf: &[]u8, offset: usize): u64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rend@1.0.0`)，许可证：MIT
