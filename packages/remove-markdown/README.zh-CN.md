[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# remove-markdown

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Markdown to plain-text stripper and excerpt generator (remove-markdown / strip-markdown parity) for search indexing, summaries, and LLM context trimming in CP

## 安装与导入

```bash
cap add remove-markdown
```

```cp
import { isSpace } from "remove-markdown";
```

## 核心导出 API

- `export fn isSpace(c: u8): bool;`
- `export fn isAlphaNum(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isAsciiPunct(c: u8): bool;`
- `export fn trimLine(s: string): string;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushTextByte(out: []u8, pos: usize, b: u8, collapse_ws: bool): usize;`
- `export fn isRuleLine(trimmed: string): bool;`
- `export fn isRefLinkDef(trimmed: string): bool;`
- `export fn detectFence(line: string): FenceInfo;`
- `export fn isClosingFence(line: string, fence_char: u8, min_len: usize): bool;`
- `export fn stripBlockPrefixes(raw_line: string): string;`
- `export fn findMatchingBracket(s: string, open_idx: usize): usize;`
- `export fn findMatchingParen(s: string, open_idx: usize): usize;`
- `export fn stripInline(s: string, opts: StripOptions, out: []u8, start_pos: usize, depth: u8): usize;`
- `export fn stripMarkdown(md: string, out: []u8): string;`
- `export fn stripMarkdownWith(md: string, opts: StripOptions, out: []u8): string;`
- `export fn stripToSingleLine(md: string, out: []u8): string;`
- `export fn excerpt(md: string, max_bytes: usize, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`remove-markdown@1.0.0`)，许可证：MIT
