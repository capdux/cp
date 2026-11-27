[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @faker-js/faker

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic fake data generator (xoshiro256** PRNG, person/network/text/commerce/UUIDv4/Luhn card/collections) for CP

## 安装与导入

```bash
cap add @faker-js/faker
```

```cp
import { splitmixStep } from "@faker-js/faker";
```

## 核心导出 API

- `export fn splitmixStep(state: &mut u64): u64;`
- `export fn rotl7(x: u64): u64;`
- `export fn rotl45(x: u64): u64;`
- `export fn appendByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn appendBytes(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendLowerAscii(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendCapitalized(out: []u8, pos: &mut usize, data: &[]u8);`
- `export fn appendU64Dec(out: []u8, pos: &mut usize, val: u64);`
- `export fn appendDecPadded(out: []u8, pos: &mut usize, val: u64, width: usize);`
- `export fn appendHexByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn isLeapYear(year: u64): bool;`
- `export fn daysInMonth(year: u64, month: u64): u64;`
- `export fn isValidLuhn(digits: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@faker-js/faker@1.0.0`)，许可证：MIT
