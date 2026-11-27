// Type definitions for CP

export fn npIsAsciiAlpha(b: u8): bool;
export fn npToUpper(b: u8): u8;
export fn npIsWinSep(b: u8): bool;
export fn npWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn npWriteSlice(out: []u8, pos: usize, src: string): usize;
export fn npSegEqDot(src: string, start: usize, len: usize): bool;
export fn npSegEqDotDot(src: string, start: usize, len: usize): bool;
export fn npNormalizePosixTwo(p1: string, p2: string, hasP2: bool, out: []u8): usize;
export fn normalizePosix(path: string, out: []u8): usize;
export fn joinNormalized(base: string, rel: string, out: []u8): usize;
export fn isVerbatimWindows(path: string): bool;
export fn stripVerbatimPrefix(path: string): string;
export fn npHasUncTag(path: string, off: usize): bool;
export fn normalizeWindows(path: string, out: []u8): usize;
export fn npWinIsAbsolute(path: string): bool;
export fn joinNormalizedWindows(base: string, rel: string, out: []u8): usize;
export fn hasParentTraversal(path: string): bool;
export fn escapesRoot(path: string): bool;
export fn isNormalized(path: string): bool;
