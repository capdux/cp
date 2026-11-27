// Type definitions for CP

export class SvgColor {
    r: u8,
    g: u8,
    b: u8,
    fn rgb(r: u8, g: u8, b: u8): SvgColor;
    fn black(): SvgColor;
    fn white(): SvgColor;
    fn red(): SvgColor;
    fn green(): SvgColor;
    fn blue(): SvgColor;
}

export fn psvgWriteByte(buf: []u8, pos: usize, b: u8): usize;
export fn psvgWriteStr(buf: []u8, pos: usize, s: string): usize;
export fn psvgWriteU32(buf: []u8, pos: usize, val: u32): usize;
export fn psvgWriteI32(buf: []u8, pos: usize, val: i32): usize;
export fn psvgWriteRgb(buf: []u8, pos: usize, c: SvgColor): usize;
export fn psvgWriteXmlEscaped(buf: []u8, pos: usize, text: string): usize;
export class SvgCanvas {
    buf: []u8,
    pos: usize,
    overflowed: bool,
    fn new(buf: []u8): SvgCanvas;
    fn written(): usize;
    fn hasOverflow(): bool;
    fn slice(): string;
    fn begin(_: auto, width: u32, height: u32): bool;
    fn end(_: auto): usize;
    fn drawRect(_: auto, x: i32, y: i32, w: u32, h: u32, fill: SvgColor, stroke_width: u32): bool;
    fn drawLine(_: auto, x1: i32, y1: i32, x2: i32, y2: i32, stroke: SvgColor, stroke_width: u32): bool;
    fn drawCircle(_: auto, cx: i32, cy: i32, r: u32, fill: SvgColor): bool;
    fn drawPolyline(_: auto, xs: &[]i32, ys: &[]i32, stroke: SvgColor, stroke_width: u32): bool;
    fn drawText(_: auto, x: i32, y: i32, text: string, font_size: u32, fill: SvgColor): bool;
}

