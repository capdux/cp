# wordwrap@1.0.0 (Agent Quickref)

> Zero-heap column-width text word wrapper with indentation, soft/hard modes, and newline preservation in pure CP | License: MIT
> Import: `import { ... } from "wordwrap";` | Full types: `src/index.d.cp`

## Functions
- `export fn wwrpIsInlineWs(c: u8): bool;`
- `export fn wwrpPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn wwrpPushSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn wwrpPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn wwrpEmitWord(word: string, start_col: usize, max_w: usize, hard_mode: bool, out: []u8, start_pos: usize, cur_line_len: usize): WwrpEmitRes;`
- `export fn wrapText(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize;`
