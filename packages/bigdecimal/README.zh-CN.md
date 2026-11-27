[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bigdecimal

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap exact base-10 financial arithmetic library with 18-digit precision, rounding, parsing, and formatting (clean-room port of Rust bigdecimal)

## 安装与导入

```bash
cap add bigdecimal
```

```cp
import { bdecIsSpace } from "bigdecimal";
```

## 核心导出 API

- `export fn bdecIsSpace(c: u8): bool;`
- `export fn bdecIsDigit(c: u8): bool;`
- `export fn bdecAbsU64(v: i64): u64;`
- `export fn bdecSignedFromU64(mag: u64, neg: bool): i64;`
- `export fn bdecPow10U64(exp: u32): u64;`
- `export fn bdecDigitCount(u: u64): i32;`
- `export fn bdecWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn parseDecimal(s: string): BigDecimal?;`
- `export fn formatDecimal(val: BigDecimal, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bigdecimal@1.0.0`)，许可证：MIT OR Apache-2.0
