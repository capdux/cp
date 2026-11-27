[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# iban_validate

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter.

## 安装与导入

```bash
cap add iban_validate
```

```cp
import { countryLength } from "iban_validate";
```

## 核心导出 API

- `export fn countryLength(c0: u8, c1: u8): usize;`
- `export fn electronicFormat(input: string, out: []u8): usize;`
- `export fn friendlyFormat(input: string, out: []u8): usize;`
- `export fn mod97(electronic: string): u32;`
- `export fn isValid(input: string): bool;`
- `export fn computeCheckDigits(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`iban_validate@1.0.0`)，许可证：MIT
