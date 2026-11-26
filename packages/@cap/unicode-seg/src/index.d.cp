// Type definitions for CP

export class DecodedRune {
    cp: u32,
    byte_len: usize,
}

export class TextSpan {
    start: usize,
    end: usize,
    slice: string,
}

export fn decodeUtf8At(s: string, offset: usize): DecodedRune;
export fn isControl(cp: u32): bool;
export fn isRegionalIndicator(cp: u32): bool;
export fn isCombiningOrExtend(cp: u32): bool;
export fn isCjkIdeograph(cp: u32): bool;
export fn isLatinWordChar(cp: u32): bool;
export fn nextGrapheme(s: string, offset: usize): TextSpan?;
export fn graphemeCount(s: string): usize;
export fn truncateGraphemes(s: string, max_graphemes: usize): string;
export fn nextWord(s: string, offset: usize): TextSpan?;
export fn wordCount(s: string): usize;
