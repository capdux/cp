// Type definitions for CP

export class FormatOptions {
    tab_width: usize,
    use_tabs: bool,
    trailing_newline: bool,
    fn defaults(): FormatOptions;
}

export fn pfIsSpace(c: u8): bool;
export fn pfPushByte(out: []u8, pos: usize, b: u8): usize;
export fn pfPushIndent(out: []u8, pos: usize, depth: usize, opts: FormatOptions): usize;
export fn pfPeekNextNonSpace(s: string, start: usize): u8;
export fn formatJson(input: string, opts: FormatOptions, out: []u8): usize;
export fn formatCss(input: string, opts: FormatOptions, out: []u8): usize;
