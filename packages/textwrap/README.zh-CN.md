[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# textwrap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation text wrapping, ANSI-aware display width, CJK wide-char breaking, indent/dedent, and truncation engine for CP

## 安装与导入

```bash
cap add textwrap
```

```cp
import { appendByte } from "textwrap";
```

## 核心导出 API

- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendSlice(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn decodeUtf8(s: string, pos: usize): DecodedChar;`
- `export fn ansiSeqLen(s: string, pos: usize): usize;`
- `export fn isAnsiReset(s: string, pos: usize, seq_len: usize): bool;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn isSpaceByte(c: u8): bool;`
- `export fn isReflowSpaceByte(c: u8): bool;`
- `export fn displayWidth(s: string): usize;`
- `export fn stripAnsi(s: string, out: []u8): string;`
- `export fn writeHardBrokenToken(text: string, tok_start: usize, tok_end: usize, width: usize, out: []u8, start_pos: usize, start_col: usize): WrapCursor;`
- `export fn wrapSegment(text: string, seg_start: usize, seg_end: usize, opts: WrapOptions, out: []u8, start_pos: usize): usize;`
- `export fn wrapTextWith(text: string, opts: WrapOptions, out: []u8): string;`
- `export fn wrapText(text: string, width: usize, out: []u8): string;`
- `export fn lineHasContent(text: string, start: usize, end: usize): bool;`
- `export fn indent(text: string, prefix: string, out: []u8): string;`
- `export fn dedent(text: string, out: []u8): string;`
- `export fn truncate(text: string, max_width: usize, ellipsis: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`textwrap@1.0.0`)（https://github.com/mgeisler/textwrap），许可证：MIT
