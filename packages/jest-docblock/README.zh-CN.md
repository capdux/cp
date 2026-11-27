[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jest-docblock

> 版本：`29.7.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap top-of-file docblock pragma extractor, parser, and formatter

## 安装与导入

```bash
cap add jest-docblock
```

```cp
import { jdblIsWs } from "jest-docblock";
```

## 核心导出 API

- `export fn jdblIsWs(c: u8): bool;`
- `export fn jdblIsLineWs(c: u8): bool;`
- `export fn jdblStrEq(a: string, b: string): bool;`
- `export fn jdblTrimLine(s: string): string;`
- `export fn jdblWriteStr(src: string, out: []u8, pos: usize): usize;`
- `export fn jdblStripDelimiters(docblock: string): string;`
- `export fn extractDocblock(code: string): string;`
- `export fn extract(code: string): string;`
- `export fn strip(code: string, out: []u8): usize;`
- `export fn stripSlice(code: string): string;`
- `export fn parseDocblock(docblock: string, out: []DocPragma): usize;`
- `export fn parse(docblock: string, out: []DocPragma): usize;`
- `export fn parseWithComments(docblock: string, comments_out: []u8, pragmas_out: []DocPragma): ParsedDocblock;`
- `export fn printDocblock(pragmas: &[]DocPragma, out: []u8): usize;`
- `export fn print(pragmas: &[]DocPragma, out: []u8): usize;`
- `export fn printWithComments(comments: string, pragmas: &[]DocPragma, out: []u8): usize;`
- `export fn getPragma(pragmas: &[]DocPragma, key: string): string?;`
- `export fn hasPragma(pragmas: &[]DocPragma, key: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jest-docblock@29.7.0`)（https://github.com/jestjs/jest），许可证：MIT
