# camelcase-keys@1.0.0 (Agent Quickref)

> CP package camelcase-keys | License: MIT
> Import: `import { ... } from "camelcase-keys";` | Full types: `src/index.d.cp`
> Deps: `ident_case`

## Functions
- `export fn ckIsSpace(ch: u8): bool;`
- `export fn ckWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn ckWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;`
- `export fn ckConvertJsonKeys(json: string, mode: u8, out: []u8): usize;`
- `export fn convertJsonKeysToCamel(json: string, out: []u8): usize;`
- `export fn convertJsonKeysToSnake(json: string, out: []u8): usize;`
- `export fn convertJsonKeysToPascal(json: string, out: []u8): usize;`
