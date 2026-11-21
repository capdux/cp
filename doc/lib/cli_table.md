# `cap:cli_table` — Zero-Alloc CLI Terminal Table Renderer

Zero-allocation pure-CP terminal table layout and border renderer supporting ASCII, compact, and Unicode light box-drawing styles, per-column alignment (`left`, `right`, `center`), East Asian wide characters, and ANSI-styled cell width calculation.

## Types

### `TableBorderStyle`

```cp
pub enum TableBorderStyle {
    ascii,
    compact,
    unicode_light,
}
```

### `CellAlign`

```cp
pub enum CellAlign {
    left,
    right,
    center,
}
```

## Exports

- `computeColumnWidths(rows: []const []const []const u8, widths_out: []usize): usize` — Computes maximum visible terminal column width per column across `rows` (ignoring ANSI CSI/OSC sequences and counting East Asian wide characters as 2 columns), writing into `widths_out` and returning the clamped column count.
- `renderBorderLine(widths: []const usize, style: TableBorderStyle, pos: u8, out: []u8): ([]const u8)?` — Renders a horizontal border line (`pos`: `0` = top, `1` = middle, `2` = bottom). For `compact`, top and bottom borders return an empty slice. Returns `null` if `pos > 2` or `out` is too small.
- `renderTableRow(cells: []const []const u8, widths: []const usize, col_aligns: []const CellAlign, style: TableBorderStyle, out: []u8): ([]const u8)?` — Renders a single row with space padding according to `widths` and `col_aligns` (missing cells default to `""`, missing alignments default to `CellAlign.left`). Returns `null` if `out` is too small.
- `renderTable(headers: []const []const u8, rows: []const []const []const u8, col_aligns: []const CellAlign, style: TableBorderStyle, out: []u8): ([]const u8)?` — Renders a complete table (top border + header + separator + data rows + bottom border) using stack storage (up to 64 columns, zero heap allocation). Returns `null` if `out` is too small.

## Example

```cp
import { TableBorderStyle, CellAlign, renderTable } from "cap:cli_table";

const headers: [2][]const u8 = ["Pkg", "Size"];
const r0: [2][]const u8 = ["cap", "12KB"];
const r1: [2][]const u8 = ["cpc", "480KB"];
const rows: [2][]const []const u8 = [r0[0..], r1[0..]];
const col_aligns: [2]CellAlign = [CellAlign.left, CellAlign.right];

let buf: [512]u8 = [0; 512];
if (rendered := renderTable(headers[0..], rows[0..], col_aligns[0..], TableBorderStyle.ascii, buf[0..])) {
    // +-----+-------+
    // | Pkg |  Size |
    // +-----+-------+
    // | cap |  12KB |
    // | cpc | 480KB |
    // +-----+-------+
}
```
