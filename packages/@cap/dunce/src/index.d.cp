// Type definitions for CP

export fn isAsciiAlpha(ch: u8): bool;
export fn toUpper(ch: u8): u8;
export fn isIllegalWinChar(ch: u8): bool;
export fn isWindowsReservedName(component: string): bool;
export fn hasWindowsVerbatimPrefix(path: string): bool;
export fn canStripVerbatimPrefix(path: string): bool;
export fn simplified(path: string, out: []u8): usize;
