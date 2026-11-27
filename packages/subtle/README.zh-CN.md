[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# subtle

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation branchless constant-time cryptographic primitives (Choice, ctEq, ctLt, ctSelect, ctCondSwap, zeroizeBytes) for CP

## 安装与导入

```bash
cap add subtle
```

```cp
import { mask8 } from "subtle";
```

## 核心导出 API

- `export fn mask8(c: Choice): u8;`
- `export fn mask32(c: Choice): u32;`
- `export fn mask64(c: Choice): u64;`
- `export fn ctIsZeroU8(x: u8): Choice;`
- `export fn ctIsZeroU32(x: u32): Choice;`
- `export fn ctIsZeroU64(x: u64): Choice;`
- `export fn ctEqU8(a: u8, b: u8): Choice;`
- `export fn ctEqU32(a: u32, b: u32): Choice;`
- `export fn ctEqU64(a: u64, b: u64): Choice;`
- `export fn ctEqBytes(a: &[]u8, b: &[]u8): Choice;`
- `export fn ctLtU64(a: u64, b: u64): Choice;`
- `export fn ctGtU64(a: u64, b: u64): Choice;`
- `export fn ctSelectU8(a: u8, b: u8, c: Choice): u8;`
- `export fn ctSelectU32(a: u32, b: u32, c: Choice): u32;`
- `export fn ctSelectU64(a: u64, b: u64, c: Choice): u64;`
- `export fn ctSelectBytes(dst: []u8, a: &[]u8, b: &[]u8, c: Choice): usize;`
- `export fn ctCondSwapU64(a: &mut u64, b: &mut u64, c: Choice);`
- `export fn zeroizeBytes(buf: []u8);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`subtle@1.0.0`)，许可证：BSD-3-Clause
- **上游生态**：`npm` (`tsscmp@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`safe-compare@1.0.0`)，许可证：MIT
