[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cli-table3

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-dependency CLI table renderer with 5 border styles (Unicode/Rounded/Ascii/Markdown/Compact), ANSI escape & CJK wide-char width calculation, truncation, and column alignment for CP

## 安装与导入

```bash
cap add cli-table3
```

```cp
import { getBorderChars } from "cli-table3";
```

## 核心导出 API

- `export enum BorderStyle`
- `export enum Align`
- `export fn getBorderChars(style: BorderStyle): BorderChars;`
- `export fn decodeUtf8(s: string, pos: usize): DecodedChar;`
- `export fn ansiSeqLen(s: string, pos: usize): usize;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn displayWidth(s: string): usize;`
- `export fn truncateDisplay(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error;`
- `export fn appendRepeat(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), chunk: string, count: usize): void ! std.mem.Allocator.Error;`
- `export fn padCell(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cli-table3@1.0.0`)，许可证：MIT
