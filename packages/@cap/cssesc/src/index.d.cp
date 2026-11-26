// Type definitions for CP

export enum CssQuoteStyle {
    Single,
    Double
}

export class CssEscOptions {
    is_identifier: bool,
    quotes: CssQuoteStyle,
    wrap: bool,
    escape_everything: bool,
}

export class CescWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,
    fn writeByte(_: auto, b: u8);
    fn writeHexEscape(_: auto, cp: u32, add_space: bool);
}

export class CescUtf8Res {
    cp: u32,
    next: usize,
}

export fn defaultCssEscOptions(): CssEscOptions;
export fn cescDecodeUtf8(s: string, start: usize): CescUtf8Res;
export fn cescIsAsciiAlnumOrDash(cp: u32): bool;
export fn cssescBuf(input: string, opts: CssEscOptions, out: []u8): usize;
export fn cssesc(input: string, opts: CssEscOptions): string;
