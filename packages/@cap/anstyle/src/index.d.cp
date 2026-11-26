// Type definitions for CP

export class Color {
    kind: u8,
    r: u8,
    g: u8,
    b: u8,
    fn none(): Color;
    fn ansi(idx: u8): Color;
    fn ansi256(idx: u8): Color;
    fn rgb(r: u8, g: u8, b: u8): Color;
}

export class Effects {
    bits: u16,
    fn new(): Effects;
    fn isEmpty(): bool;
}

export fn writeByte(out: []u8, pos: usize, b: u8): usize;
export fn writeU8Dec(out: []u8, pos: usize, val: u8): usize;
export fn writeParam(out: []u8, pos: usize, has_prev: bool, val: u8): usize;
export class Style {
    fg: Color,
    bg: Color,
    effects: Effects,
    fn new(): Style;
    fn isPlain(): bool;
    fn fgColor(c: Color): Style;
    fn bgColor(c: Color): Style;
    fn bold(): Style;
    fn dimmed(): Style;
    fn italic(): Style;
    fn underline(): Style;
    fn blink(): Style;
    fn invert(): Style;
    fn hidden(): Style;
    fn strikethrough(): Style;
    fn renderStart(out: []u8): usize;
    fn renderReset(out: []u8): usize;
    fn paint(text: string, out: []u8): usize;
}

