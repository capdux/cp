// Type definitions for CP

export fn isExtendedLengthPath(path: string): bool;
export fn slash(path: string, out: []u8): usize;
export fn toBackslash(path: string, out: []u8): usize;
export fn countSeparators(path: string): usize;
