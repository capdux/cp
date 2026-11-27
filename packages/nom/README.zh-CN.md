[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nom

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation streaming parser combinator primitives (tag, tagNoCase, take, takeUntil, character classes, delimited/separated sequences, integer parsers) for CP (clean-room MIT, inspired by Rust nom v7).

## 安装与导入

```bash
cap add nom
```

```cp
import { nomFail } from "nom";
```

## 核心导出 API

- `export fn nomFail(input: string): ParseRes;`
- `export fn nomOk(input: string, split_at: usize): ParseRes;`
- `export fn nomContainsByte(set: string, b: u8): bool;`
- `export fn nomIsDigit(b: u8): bool;`
- `export fn nomIsHexDigit(b: u8): bool;`
- `export fn nomHexVal(b: u8): u64;`
- `export fn nomIsAlpha(b: u8): bool;`
- `export fn nomIsAlnum(b: u8): bool;`
- `export fn nomIsSpace(b: u8): bool;`
- `export fn nomIsMultispace(b: u8): bool;`
- `export fn nomSliceEq(a: string, b: string): bool;`
- `export fn tag(input: string, expected: string): ParseRes;`
- `export fn tagNoCase(input: string, expected: string): ParseRes;`
- `export fn take(input: string, count: usize): ParseRes;`
- `export fn takeUntil(input: string, needle: string): ParseRes;`
- `export fn isNot(input: string, forbidden_chars: string): ParseRes;`
- `export fn isA(input: string, allowed_chars: string): ParseRes;`
- `export fn digit0(input: string): ParseRes;`
- `export fn digit1(input: string): ParseRes;`
- `export fn hexDigit1(input: string): ParseRes;`
- `export fn alpha0(input: string): ParseRes;`
- `export fn alpha1(input: string): ParseRes;`
- `export fn alphanumeric0(input: string): ParseRes;`
- `export fn alphanumeric1(input: string): ParseRes;`
- `export fn space0(input: string): ParseRes;`
- `export fn space1(input: string): ParseRes;`
- `export fn multispace0(input: string): ParseRes;`
- `export fn multispace1(input: string): ParseRes;`
- `export fn lineEnding(input: string): ParseRes;`
- `export fn notLineEnding(input: string): ParseRes;`
- `export fn delimitedTag(input: string, open: string, close: string): ParseRes;`
- `export fn precededTag(input: string, prefix: string): ParseRes;`
- `export fn terminatedTag(input: string, suffix: string): ParseRes;`
- `export fn separatedPairTag(input: string, sep: string): ParseRes;`
- `export fn parseU64(input: string): UintRes;`
- `export fn parseI64(input: string): IntRes;`
- `export fn parseHexU64(input: string): UintRes;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`nom@1.0.0`)，许可证：MIT
