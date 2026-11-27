// Type definitions for CP

export fn siIsIndentChar(ch: u8): bool;
export fn siCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;
export fn minIndent(s: string): usize;
export fn stripIndentBy(s: string, n: usize, out: []u8): usize;
export fn stripIndent(s: string, out: []u8): usize;
export fn stripIndentInto(s: string, out: []u8): usize;
export class StripIndentOptions {
    tab_width: usize,
    trim_blank_lines: bool,
    fn defaults(): StripIndentOptions;
}

export fn minIndentWithTabWidth(s: string, tab_width: usize): usize;
export fn stripIndentWith(s: string, opts: StripIndentOptions, out: []u8): usize;
export fn indentLines(s: string, count: usize, indent_char: u8, include_empty: bool, out: []u8): usize;
