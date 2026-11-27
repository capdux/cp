[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ident_case

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap case conversion engine supporting camelCase, PascalCase, snake_case, kebab-case, and SCREAMING_SNAKE_CASE with smart acronym splitting.

## 安装与导入

```bash
cap add ident_case
```

```cp
import { isUpper } from "ident_case";
```

## 核心导出 API

- `export fn isUpper(c: u8): bool;`
- `export fn isLower(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isAlnum(c: u8): bool;`
- `export fn toLower(c: u8): u8;`
- `export fn toUpper(c: u8): u8;`
- `export fn nextWord(input: string, pos: usize, out_start: &mut usize, out_end: &mut usize): bool;`
- `export fn convertJoined(input: string, out: []u8, sep: u8, has_sep: bool, upper_all: bool, cap_first_word: bool, cap_rest_words: bool): usize;`
- `export fn toCamelCase(input: string, out: []u8): usize;`
- `export fn toPascalCase(input: string, out: []u8): usize;`
- `export fn toSnakeCase(input: string, out: []u8): usize;`
- `export fn toKebabCase(input: string, out: []u8): usize;`
- `export fn toScreamingSnakeCase(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ident_case@1.0.0`)，许可证：MIT
