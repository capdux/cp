[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# indoc

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation multiline string dedent, indent, stripMargin, and newline normalization engine for CP

## 安装与导入

```bash
cap add indoc
```

```cp
import { isHws } from "indoc";
```

## 核心导出 API

- `export fn isHws(c: u8): bool;`
- `export fn isLineBlank(s: string, start: usize, end: usize): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendSlice(out: []u8, start_pos: usize, s: string, a: usize, b: usize): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn stripOuterSingleBlankLines(input: string): string;`
- `export fn trimBlankLines(input: string): string;`
- `export fn minCommonIndent(input: string): usize;`
- `export fn dedent(input: string, out: []u8): string;`
- `export fn indent(input: string, prefix: string, skip_empty: bool, out: []u8): string;`
- `export fn stripMargin(input: string, margin_char: u8, out: []u8): string;`
- `export fn normalizeNewlines(input: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`indoc@1.0.0`)，许可证：MIT
