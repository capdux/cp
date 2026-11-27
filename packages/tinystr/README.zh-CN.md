[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tinystr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation stack-inline fixed-capacity ASCII strings (TinyAsciiStr4, TinyAsciiStr8, TinyAsciiStr16) with u32/u64 packing and BCP-47 case transforms (ICU4X tinystr parity) in pure CP.

## 安装与导入

```bash
cap add tinystr
```

```cp
import { tsToLowerByte } from "tinystr";
```

## 核心导出 API

- `export fn tsToLowerByte(b: u8): u8;`
- `export fn tsToUpperByte(b: u8): u8;`
- `export fn tsIsAlpha(b: u8): bool;`
- `export fn tsIsDigit(b: u8): bool;`
- `export fn tsIsAlnum(b: u8): bool;`
- `export fn tsIsLower(b: u8): bool;`
- `export fn tsIsUpper(b: u8): bool;`
- `export fn tsValidateBytes(src: &[]u8, max_len: usize): i64;`
- `export fn tsValidatePacked(buf: &[]u8): i64;`
- `export fn tsSliceLen(buf: &[]u8): usize;`
- `export fn tsCopyOut(src: &[]u8, n: usize, out: []u8): usize;`
- `export fn tsSliceEq(a: &[]u8, b: &[]u8): bool;`
- `export fn tsSliceCmp(a: &[]u8, b: &[]u8): i32;`
- `export fn tsSliceEqIgnoreCase(a: &[]u8, b: &[]u8): bool;`
- `export fn tsAllAlpha(buf: &[]u8, n: usize): bool;`
- `export fn tsAllAlnum(buf: &[]u8, n: usize): bool;`
- `export fn tsAllDigit(buf: &[]u8, n: usize): bool;`
- `export fn tsAllLower(buf: &[]u8, n: usize): bool;`
- `export fn tsAllUpper(buf: &[]u8, n: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tinystr@1.0.0`)，许可证：Unicode-3.0
