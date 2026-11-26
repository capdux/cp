// Type definitions for CP

export fn stripFinalNewline(input: string): string;
export fn stripAllTrailingNewlines(input: string): string;
export fn hasFinalNewline(input: string): bool;
export fn hasFinalCrlf(input: string): bool;
export fn ensureFinalNewline(input: string, use_crlf: bool, out: []u8): usize;
