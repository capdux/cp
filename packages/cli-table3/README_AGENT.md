# cli-table3@1.0.0 (Agent Quickref)

> Zero-dependency CLI table renderer with 5 border styles (Unicode/Rounded/Ascii/Markdown/Compact), ANSI escape & CJK wide-char width calculation, truncation, and column alignment for CP | License: MIT
> Import: `import { ... } from "cli-table3";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum BorderStyle`
- `export enum Align`

## Functions
- `export fn getBorderChars(style: BorderStyle): BorderChars;`
- `export fn decodeUtf8(s: string, pos: usize): DecodedChar;`
- `export fn ansiSeqLen(s: string, pos: usize): usize;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn displayWidth(s: string): usize;`
- `export fn truncateDisplay(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error;`
- `export fn appendRepeat(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), chunk: string, count: usize): void ! std.mem.Allocator.Error;`
- `export fn padCell(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error;`
