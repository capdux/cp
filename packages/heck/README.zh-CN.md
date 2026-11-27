[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# heck

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Rust heck-compatible case conversion engine (PascalCase, camelCase, snake_case, SHOUTY_SNAKE_CASE, kebab-case, SHOUTY-KEBAB-CASE, Title Case, Train-Case) for CP (clean-room MIT).

## 安装与导入

```bash
cap add heck
```

```cp
import { hckDecodeUtf8 } from "heck";
```

## 核心导出 API

- `export fn hckDecodeUtf8(src: string, offset: usize): HckRune;`
- `export fn hckClassify(cp: u32): u8;`
- `export fn hckToLowerCp(cp: u32): u32;`
- `export fn hckToUpperCp(cp: u32): u32;`
- `export fn hckEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn hckEmitWord(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize;`
- `export fn hckConvert(src: string, out: []u8, sep: u8, style: u8): usize;`
- `export fn toUpperCamelCase(src: string, out: []u8): usize;`
- `export fn toLowerCamelCase(src: string, out: []u8): usize;`
- `export fn toSnakeCase(src: string, out: []u8): usize;`
- `export fn toShoutySnakeCase(src: string, out: []u8): usize;`
- `export fn toKebabCase(src: string, out: []u8): usize;`
- `export fn toShoutyKebabCase(src: string, out: []u8): usize;`
- `export fn toTitleCase(src: string, out: []u8): usize;`
- `export fn toTrainCase(src: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`heck@1.0.0`)，许可证：MIT
