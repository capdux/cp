[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ascii

> 版本：`1.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ASCII validation, case conversion, case-insensitive comparison, and byte classification

## 安装与导入

```bash
cap add ascii
```

```cp
import { isAsciiAlpha } from "ascii";
```

## 核心导出 API

- `export fn isAsciiAlpha(b: u8): bool;`
- `export fn isAsciiDigit(b: u8): bool;`
- `export fn isAsciiAlphanumeric(b: u8): bool;`
- `export fn isAsciiHexDigit(b: u8): bool;`
- `export fn isAsciiWhitespace(b: u8): bool;`
- `export fn isAsciiPrintable(b: u8): bool;`
- `export fn isAsciiControl(b: u8): bool;`
- `export fn toAsciiLowerByte(b: u8): u8;`
- `export fn toAsciiUpperByte(b: u8): u8;`
- `export fn eqIgnoreAsciiCase(a: string, b: string): bool;`
- `export fn toAsciiLower(s: string, out: []u8): usize;`
- `export fn toAsciiUpper(s: string, out: []u8): usize;`
- `export fn trimAscii(s: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ascii@1.1.0`)（https://github.com/tomprogrammer/rust-ascii），许可证：MIT
