// Type definitions for CP

export fn writeStr(out: []u8, pos: usize, s: string): usize;
export fn matchAt(input: string, pos: usize, pat: string): bool;
export fn needsHtmlEscape(input: string): bool;
export fn escapedHtmlLen(input: string): usize;
export fn escapeHtml(input: string, out: []u8): usize;
export fn unescapeHtml(input: string, out: []u8): usize;
