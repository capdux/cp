# markdown-table — AI Agent Quick Ref

Zero-heap GFM Markdown table renderer with automatic column width calculation and alignment padding (`v3.0.4`, `MIT`)

## Import

```cp
import {
    TableAlign,
    mdtbWriteByte,
    mdtbWriteStr,
    mdtbWriteRepeat,
    mdtbGetAlign,
    mdtbMinWidth,
    mdtbWriteCell,
    mdtbWriteDelim,
} from "markdown-table";
```

## Core Types & Constants

```cp
enum TableAlign { None, Left, Center, Right }
```

## API Signatures

```cp
fn mdtbWriteByte(out: []u8, pos: usize, ch: u8): usize;
fn mdtbWriteStr(out: []u8, pos: usize, s: string): usize;
fn mdtbWriteRepeat(out: []u8, pos: usize, ch: u8, count: usize): usize;
fn mdtbGetAlign(aligns: &[]TableAlign, col: usize): TableAlign;
fn mdtbMinWidth(al: TableAlign): usize;
fn mdtbWriteCell(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize;
fn mdtbWriteDelim(out: []u8, pos: usize, width: usize, al: TableAlign): usize;
fn mdtbColWidth(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, col: usize, al: TableAlign): usize;
fn renderMarkdownTable(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, aligns: &[]TableAlign, out: []u8): usize;
```

## Minimal Usage

```cp
import { mdtbWriteStr } from "markdown-table";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mdtbWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"markdown-table"`; install via `cap add markdown-table`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
