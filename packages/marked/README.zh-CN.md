[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# marked

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CommonMark/GFM Markdown to HTML compiler and plain-text stripper in pure CP with XSS-safe HTML escaping via @cap/escape-html (clean-room MIT counterpart to npm marked v14).

## 安装与导入

```bash
cap add marked
```

```cp
import { defaultMarkedOptions } from "marked";
```

## 核心导出 API

- `export fn defaultMarkedOptions(): MarkedOptions;`
- `export fn mkdWriteByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn mkdWriteStr(out: []u8, pos: &mut usize, s: string);`
- `export fn mkdWriteEscaped(out: []u8, pos: &mut usize, s: string);`
- `export fn mkdWriteEscapedByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn mkdIsSpace(c: u8): bool;`
- `export fn mkdIsPunctEscape(c: u8): bool;`
- `export fn mkdStartsWithIgnoreCaseClean(url: string, prefix: string): bool;`
- `export fn mkdIsSafeUrl(url: string): bool;`
- `export fn mkdFindLineEnd(s: string, start: usize): usize;`
- `export fn mkdTrimCr(s: string, start: usize, end: usize): usize;`
- `export fn mkdSkipIndent(s: string, start: usize, end: usize, max_spaces: usize): usize;`
- `export fn mkdIsBlankLine(s: string, start: usize, end: usize): bool;`
- `export fn mkdMatchHeading(s: string, line_start: usize, line_end: usize, content_start: &mut usize, content_end: &mut usize): usize;`
- `export fn mkdIsHorizontalRule(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdMatchFenceOpen(s: string, line_start: usize, line_end: usize, fence_char: &mut u8, lang_start: &mut usize, lang_end: &mut usize): usize;`
- `export fn mkdIsFenceClose(s: string, line_start: usize, line_end: usize, fence_char: u8, min_len: usize): bool;`
- `export fn mkdIsBlockquoteLine(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdMatchUnorderedItem(s: string, line_start: usize, line_end: usize): usize;`
- `export fn mkdMatchOrderedItem(s: string, line_start: usize, line_end: usize, num_out: &mut usize): usize;`
- `export fn mkdStartsNewBlock(s: string, line_start: usize, line_end: usize): bool;`
- `export fn mkdFindClosingDouble(s: string, start: usize, ch: u8): i64;`
- `export fn mkdFindClosingSingle(s: string, start: usize, ch: u8): i64;`
- `export fn mkdMatchLinkOrImage(s: string, open_bracket: usize, label_start: &mut usize, label_end: &mut usize, url_start: &mut usize, url_end: &mut usize): usize;`
- `export fn mkdIsInlineSpecial(c: u8): bool;`
- `export fn mkdParseInlineRec(src: string, opts: MarkedOptions, out: []u8, pos: &mut usize, depth: usize);`
- `export fn mkdStripBlockquoteLines(md: string, start: usize, end: usize, dst: []u8): usize;`
- `export fn mkdParseBlocksRec(md: string, opts: MarkedOptions, out: []u8, pos: &mut usize, depth: usize);`
- `export fn mkdStripInlineRec(src: string, out: []u8, pos: &mut usize, depth: usize);`
- `export fn parseMarkdown(md: string, out: []u8): usize;`
- `export fn parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize;`
- `export fn parseInlineMarkdown(src: string, out: []u8): usize;`
- `export fn stripMarkdown(md: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`marked@1.0.0`)，许可证：MIT
