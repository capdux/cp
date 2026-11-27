// Type definitions for CP

export fn isExtglobPrefix(ch: u8): bool;
export fn matchExtglobEnd(str: string, start: usize): usize;
export fn findExtglobStart(str: string): i64;
export fn isExtglob(str: string): bool;
export fn countExtglobs(str: string): usize;
