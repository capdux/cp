// Type definitions for CP

export enum BorderStyle {
    Unicode = 0,
    Rounded = 1,
    Ascii = 2,
    Markdown = 3,
    Compact = 4
}

export enum Align {
    Left = 0,
    Center = 1,
    Right = 2
}

export class BorderChars {
    top_left: string,
    top_mid: string,
    top_right: string,
    top_horiz: string,
    mid_left: string,
    mid_mid: string,
    mid_right: string,
    mid_horiz: string,
    bot_left: string,
    bot_mid: string,
    bot_right: string,
    bot_horiz: string,
    left_vert: string,
    mid_vert: string,
    right_vert: string,
}

export fn getBorderChars(style: BorderStyle): BorderChars;
export class DecodedChar {
    cp: u32,
    len: usize,
}

export fn decodeUtf8(s: string, pos: usize): DecodedChar;
export fn ansiSeqLen(s: string, pos: usize): usize;
export fn codepointWidth(cp: u32): usize;
export fn displayWidth(s: string): usize;
export fn truncateDisplay(alloc: std.mem.Allocator, s: string, max_width: usize): []u8 ! std.mem.Allocator.Error;
export fn appendRepeat(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), chunk: string, count: usize): void ! std.mem.Allocator.Error;
export fn padCell(alloc: std.mem.Allocator, s: string, width: usize, alignment: Align): []u8 ! std.mem.Allocator.Error;
export class Table {
    alloc: std.mem.Allocator,
    headers: std.ArrayList(string),
    rows: std.ArrayList([]string),
    aligns: std.ArrayList(Align),
    style: BorderStyle,
    max_col_width: usize,
    fn init(alloc: std.mem.Allocator): Table;
    fn deinit(_: auto);
    fn setHeaders(_: auto, headers: &[]string): void ! std.mem.Allocator.Error;
    fn addRow(_: auto, row: &[]string): void ! std.mem.Allocator.Error;
    fn setAlign(_: auto, col: usize, alignment: Align): void ! std.mem.Allocator.Error;
    fn setStyle(_: auto, style: BorderStyle);
    fn setMaxColWidth(_: auto, max_width: usize);
    fn colAlign(col: usize): Align;
    fn computeColCount(): usize;
    fn renderHorizBorder(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), left: string, mid: string, right: string, horiz: string, widths: &[]usize, padded: bool): void ! std.mem.Allocator.Error;
    fn renderMarkdownSep(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), widths: &[]usize): void ! std.mem.Allocator.Error;
    fn renderDataRow(alloc: std.mem.Allocator, out: &mut std.ArrayList(u8), bc: BorderChars, cells: &[]string, widths: &[]usize, is_compact: bool): void ! std.mem.Allocator.Error;
    fn render(alloc: std.mem.Allocator): []u8 ! std.mem.Allocator.Error;
}

