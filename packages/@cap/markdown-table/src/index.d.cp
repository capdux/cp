// Type definitions for CP

export enum TableAlign {
    None,
    Left,
    Center,
    Right
}

export fn mdtbWriteByte(out: []u8, pos: usize, ch: u8): usize;
export fn mdtbWriteStr(out: []u8, pos: usize, s: string): usize;
export fn mdtbWriteRepeat(out: []u8, pos: usize, ch: u8, count: usize): usize;
export fn mdtbGetAlign(aligns: &[]TableAlign, col: usize): TableAlign;
export fn mdtbMinWidth(al: TableAlign): usize;
export fn mdtbWriteCell(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize;
export fn mdtbWriteDelim(out: []u8, pos: usize, width: usize, al: TableAlign): usize;
export fn mdtbColWidth(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, col: usize, al: TableAlign): usize;
export fn renderMarkdownTable(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, aligns: &[]TableAlign, out: []u8): usize;
