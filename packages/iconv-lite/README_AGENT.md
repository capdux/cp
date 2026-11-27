# iconv-lite@1.0.0 (Agent Quickref)

> Zero-allocation character encoding converter supporting UTF-8, ASCII, Latin1, Windows-1252, UTF-16LE/BE with BOM and surrogate pairs for CP | License: MIT
> Import: `import { ... } from "iconv-lite";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Charset`

## Functions
- `export fn icvStrEq(a: string, b: string): bool;`
- `export fn parseCharset(name: string): Charset;`
- `export fn encodingExists(name: string): bool;`
- `export fn icvWin1252ToUnicode(b: u8): u32;`
- `export fn icvUnicodeToWin1252(cp: u32): u8?;`
- `export fn icvWriteUtf8(out: []u8, pos: usize, cp: u32): usize;`
- `export fn icvReadUtf8(s: string, pos: usize): IcvUtf8Step;`
- `export fn icvReadU16(data: &[]u8, pos: usize, is_le: bool): u16;`
- `export fn icvWriteU16(out: []u8, pos: usize, val: u16, is_le: bool): usize;`
- `export fn decodeToUtf8(data: &[]u8, charset: Charset, strip_bom: bool, out: []u8): usize;`
- `export fn encodeFromUtf8(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize;`
