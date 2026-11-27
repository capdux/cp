# ndjson@1.0.0 (Agent Quickref)

> Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer with max-line overflow protection. | License: MIT
> Import: `import { ... } from "ndjson";` | Full types: `src/index.d.cp`

## Functions
- `export fn isBlankSlice(s: &[]u8): bool;`
- `export fn nextLine(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine;`
- `export fn countLines(src: &[]u8): usize;`
- `export fn writeLine(json_record: &[]u8, dst: []u8): i64;`
