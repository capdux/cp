# pad@1.0.0 (Agent Quickref)

> Zero-allocation UTF-8 codepoint-aware string padding (padStart, padEnd, padCenter) and repeatStr in pure CP | License: MIT
> Import: `import { ... } from "pad";` | Full types: `src/index.d.cp`

## Functions
- `export fn countChars(s: string): usize;`
- `export fn copyBytes(dst: []u8, pos: usize, src: string);`
- `export fn fillByte(dst: []u8, pos: usize, count: usize, ch: u8);`
- `export fn padStart(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn repeatStr(s: string, count: usize, out: []u8): usize;`
