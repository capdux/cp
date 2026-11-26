// Type definitions for CP

export fn esrIsSpecial(c: u8): bool;
export fn hasRegexpMeta(input: string): bool;
export fn escapedRegexpLen(input: string): usize;
export fn escapeStringRegexp(input: string, out: []u8): usize;
export fn unescapeStringRegexp(escaped: string, out: []u8): usize;
