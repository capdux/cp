// Type definitions for CP

export fn isFullwidthCodePoint(cp: u32): bool;
export fn isZeroWidthCodePoint(cp: u32): bool;
export fn codePointWidth(cp: u32): usize;
export fn countFullwidthChars(s: string): usize;
export fn hasFullwidthChar(s: string): bool;
export fn stringDisplayWidth(s: string): usize;
export fn truncateToDisplayWidth(s: string, max_width: usize): string;
