[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cast

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation checked primitive numeric casts preventing silent overflow, underflow, and NaN/Inf truncation (clean-room MIT implementation of crates.io cast, #443 / 335M+)

## 安装与导入

```bash
cap add cast
```

```cp
import { u8FromI64 } from "cast";
```

## 核心导出 API

- `export enum CastError`
- `export fn u8FromI64(v: i64): u8?;`
- `export fn u8FromU64(v: u64): u8?;`
- `export fn u16FromI64(v: i64): u16?;`
- `export fn u32FromI64(v: i64): u32?;`
- `export fn u32FromU64(v: u64): u32?;`
- `export fn i32FromU64(v: u64): i32?;`
- `export fn i32FromI64(v: i64): i32?;`
- `export fn u64FromI64(v: i64): u64?;`
- `export fn i64FromU64(v: u64): i64?;`
- `export fn usizeFromI64(v: i64): usize?;`
- `export fn u32FromF64(v: f64): u32?;`
- `export fn i32FromF64(v: f64): i32?;`
- `export fn u8FromF64(v: f64): u8?;`
- `export fn checkF64ToI32(v: f64): CastError?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cast@1.0.0`)，许可证：MIT OR Apache-2.0
