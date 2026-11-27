[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-conv

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap lossless widening, explicit truncating, checked, and saturating integer conversions matching Rust num-conv in pure CP

## 安装与导入

```bash
cap add num-conv
```

```cp
import { extendU8ToU64 } from "num-conv";
```

## 核心导出 API

- `export fn extendU8ToU64(v: u8): u64;`
- `export fn extendU16ToU64(v: u16): u64;`
- `export fn extendU32ToU64(v: u32): u64;`
- `export fn extendI8ToI64(v: i8): i64;`
- `export fn extendI16ToI64(v: i16): i64;`
- `export fn extendI32ToI64(v: i32): i64;`
- `export fn truncateU64ToU8(v: u64): u8;`
- `export fn truncateU64ToU16(v: u64): u16;`
- `export fn truncateU64ToU32(v: u64): u32;`
- `export fn tryU64ToU8(v: u64): CheckedU8;`
- `export fn tryU64ToU16(v: u64): CheckedU16;`
- `export fn tryU64ToU32(v: u64): CheckedU32;`
- `export fn tryI64ToI8(v: i64): CheckedI8;`
- `export fn tryI64ToI16(v: i64): CheckedI16;`
- `export fn tryI64ToI32(v: i64): CheckedI32;`
- `export fn tryI64ToU8(v: i64): CheckedU8;`
- `export fn tryI64ToU32(v: i64): CheckedU32;`
- `export fn saturatingI64ToU8(v: i64): u8;`
- `export fn saturatingI64ToI8(v: i64): i8;`
- `export fn saturatingI64ToI16(v: i64): i16;`
- `export fn saturatingU64ToU32(v: u64): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-conv@1.0.0`)，许可证：MIT
