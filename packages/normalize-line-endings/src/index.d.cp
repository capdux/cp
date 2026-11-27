// Type definitions for CP

export enum LineEnding {
    None,
    Lf,
    Crlf,
    Cr,
    Mixed
}

export fn detectLineEnding(input: string): LineEnding;
export fn normalizeToLf(input: string, out: []u8): usize;
export fn normalizeToCrlf(input: string, out: []u8): usize;
