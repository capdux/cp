// Type definitions for CP

export enum LineEnding {
    None,
    Lf,
    Crlf,
    Cr,
    Mixed,
}

export fn lineEndingStr(le: LineEnding): string;
export fn detectLineEnding(input: string): LineEnding;
export fn countLineBreaks(input: string): usize;
export fn countLines(input: string): usize;
export fn needsNormalization(input: string, target: LineEnding): bool;
export fn normalizedLen(input: string, target: LineEnding): usize;
export fn normalizeToLf(input: string, out: []u8): usize;
export fn normalizeToCrlf(input: string, out: []u8): usize;
export fn normalizeToCr(input: string, out: []u8): usize;
export fn normalizeLineEndings(input: string, target: LineEnding, out: []u8): usize;
