// Type definitions for CP

export class NewlineStats {
    lf_count: usize,
    crlf_count: usize,
    cr_count: usize,
    dominant: u8,
}

export fn analyzeNewlines(input: string): NewlineStats;
export fn detectNewline(input: string): string;
export fn detectNewlineGraceful(input: string): string;
export fn normalizeToLf(input: string, out: []u8): usize;
export fn normalizeToCrlf(input: string, out: []u8): usize;
