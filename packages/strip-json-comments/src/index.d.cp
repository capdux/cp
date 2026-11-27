// Type definitions for CP

export class StripOptions {
    preserve_whitespace: bool,
    strip_trailing_commas: bool,
}

export fn isSpace(c: u8): bool;
export fn isTrailingComma(src: &[]u8, commaIdx: usize): bool;
export fn stripComments(src: &[]u8, dst: []u8, opts: StripOptions): i64;
export fn stripJsonc(src: &[]u8, dst: []u8): i64;
export fn stripCompact(src: &[]u8, dst: []u8): i64;
