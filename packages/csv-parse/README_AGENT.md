# csv-parse@1.0.0 (Agent Quickref)

> Zero-heap RFC 4180 CSV/TSV record parser and serializer with quoted fields, escaped quotes, embedded newlines, custom delimiters, and spreadsheet formula guard. | License: MIT
> Import: `import { ... } from "csv-parse";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseRecord(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult;`
- `export fn formatField(field: &[]u8, delim: u8, dst: []u8): i64;`
- `export fn formatFieldSafe(field: &[]u8, delim: u8, dst: []u8): i64;`
- `export fn formatRecord(fields: &[]&[]u8, delim: u8, dst: []u8): i64;`
