# toml_edit@1.0.0 (Agent Quickref)

> Zero-allocation TOML v1.0 document parser, typed query/mutation engine, and canonical formatter (Rust toml_edit v0.22 parity) for CP | License: MIT
> Import: `import { ... } from "toml_edit";` | Full types: `src/index.d.cp`

## Functions
- `export fn tmeStripComment(s: string): string;`
- `export fn tmeParseInt(raw: string): i64?;`
- `export fn tmeWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn tmeWriteQuotedStr(out: []u8, start_pos: usize, s: string): usize;`
- `export fn tmeWriteEntry(out: []u8, start_pos: usize, e: TomlEntry): usize;`
