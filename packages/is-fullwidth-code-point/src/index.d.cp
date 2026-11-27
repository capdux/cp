// Type definitions for CP

export fn isFullwidthCodePoint(cp: u32): bool;
export fn codePointWidth(cp: u32): usize;
export fn countFullwidthChars(s: string): usize;
