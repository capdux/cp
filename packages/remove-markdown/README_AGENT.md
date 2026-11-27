# remove-markdown@1.0.0 (Agent Quickref)

> Zero-allocation Markdown to plain-text stripper and excerpt generator (remove-markdown / strip-markdown parity) for search indexing, summaries, and LLM context trimming in CP | License: MIT
> Import: `import { ... } from "remove-markdown";` | Full types: `src/index.d.cp`

## Functions
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
