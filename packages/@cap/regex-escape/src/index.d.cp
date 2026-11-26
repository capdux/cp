// Type definitions for CP

export fn isMetaChar(c: u8): bool;
export fn isEscapeTarget(c: u8): bool;
export fn needsEscape(input: string): bool;
export fn escapedLen(input: string): usize;
export fn escapeRegex(input: string, out: []u8): usize;
export fn escapeCharClass(input: string, out: []u8): usize;
export fn unescapeRegexLiteral(input: string, out: []u8): usize;
