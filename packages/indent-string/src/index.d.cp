// Type definitions for CP

export fn writeRepeat(out: []u8, pos: usize, unit: string, count: usize): usize;
export fn isLineBlank(input: string, start: usize, end: usize): bool;
export fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize;
export fn indentString(input: string, count: usize, out: []u8): usize;
