// Type definitions for CP

export fn pclByteAt(a: string, b: string, has_sep: bool, idx: usize): u8;
export fn pclCleanTwo(a: string, b: string, has_sep: bool, out: []u8): usize;
export fn cleanPath(path: string, out: []u8): usize;
export fn cleanJoin(a: string, b: string, out: []u8): usize;
export fn isClean(path: string): bool;
