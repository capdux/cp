[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# litrs

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Rust literal lexer, validator, and string unescaper (Rust litrs parity) for CP

## 安装与导入

```bash
cap add litrs
```

```cp
import { ltrInvalid } from "litrs";
```

## 核心导出 API

- `export enum LiteralKind`
- `export fn ltrInvalid(src: string): ParsedLiteral;`
- `export fn ltrStrEq(a: string, b: string): bool;`
- `export fn ltrIsIdentStart(c: u8): bool;`
- `export fn ltrIsIdentCont(c: u8): bool;`
- `export fn ltrIsValidSuffix(s: string): bool;`
- `export fn ltrHexVal(c: u8): i32;`
- `export fn ltrDigitVal(c: u8, radix: u64): i32;`
- `export fn ltrParseRawString(src: string, start: usize, kind: LiteralKind): ParsedLiteral;`
- `export fn ltrParseQuotedString(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral;`
- `export fn ltrParseCharOrByte(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral;`
- `export fn ltrParseNumber(src: string): ParsedLiteral;`
- `export fn parseLiteral(src: string): ParsedLiteral;`
- `export fn ltrEncodeUtf8(cp: u32, out: []u8, pos: usize): usize?;`
- `export fn unescapeString(lit_raw: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`litrs@1.0.0`)，许可证：MIT OR Apache-2.0
