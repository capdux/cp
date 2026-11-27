// Type definitions for CP

export fn isHorizSpace(c: u8): bool;
export fn minIndent(input: string): usize;
export fn stripIndent(input: string, out: []u8): usize;
export fn indentLines(input: string, spaces: usize, out: []u8): usize;
