# `cap:markdown_table` — GFM Pipe Table Parser & Escaper

Zero-allocation GitHub Flavored Markdown (GFM) pipe-table row cell iterator, delimiter alignment detector, and cell escaper/unescaper.

## Exports

- `ColAlign` (`none` | `left` | `center` | `right`)
- `parseDelimiterCell(raw_cell): ColAlign?`
- `TableRowIter` (`init(raw_line)`, `next(*_): ([]const u8)?`)
- `parseDelimiterRow(raw_line, out_aligns: []ColAlign): usize?`
- `unescapeTableCell(raw_cell, out): ([]const u8)?` / `escapeTableCell(text, out): ([]const u8)?`
