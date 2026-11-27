// Type definitions for CP

export fn esrIsSpecial(c: u8): bool;
export fn isRegexpMeta(c: u8): bool;
export fn hasRegexpMeta(input: string): bool;
export fn needsRegexEscape(input: string): bool;
export fn escapedRegexpLen(input: string): usize;
export fn escapedLen(input: string): usize;
export fn escapeStringRegexp(input: string, out: []u8): usize;
export fn escapeStringRegexpChecked(input: string, out: []u8): usize?;
export fn escapeCharClass(input: string, out: []u8): usize;
export fn unescapedLen(escaped: string): usize;
export fn unescapeStringRegexp(escaped: string, out: []u8): usize;
export fn unescapeRegexp(escaped: string, out: []u8): usize;
